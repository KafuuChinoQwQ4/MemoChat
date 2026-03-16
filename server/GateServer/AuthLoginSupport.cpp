#include "AuthLoginSupport.h"

#include "ConfigMgr.h"
#include "PostgresDao.h"
#include "RedisMgr.h"
#include "auth/ChatLoginTicket.h"
#include "cluster/ChatClusterDiscovery.h"
#include "logging/Logger.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <cctype>
#include <json/json.h>
#include <sstream>

namespace {
constexpr const char* kMinClientVersion = "2.0.0";
constexpr int kLoginProtocolVersion = 3;
std::atomic<uint64_t> g_gate_route_rr_counter{0};

bool IsTruthyFlag(const char* raw) {
    if (raw == nullptr) {
        return false;
    }
    const std::string value(raw);
    return value == "1" || value == "true" || value == "TRUE" || value == "on" || value == "ON";
}

bool ParseSemVer(const std::string& ver, int& major, int& minor, int& patch) {
    major = 0;
    minor = 0;
    patch = 0;
    if (ver.empty()) {
        return false;
    }

    std::stringstream ss(ver);
    std::string token;
    std::vector<int> parts;
    while (std::getline(ss, token, '.')) {
        if (token.empty()) {
            return false;
        }
        for (char c : token) {
            if (!std::isdigit(static_cast<unsigned char>(c))) {
                return false;
            }
        }
        parts.push_back(std::stoi(token));
    }
    if (parts.empty() || parts.size() > 3) {
        return false;
    }
    major = parts[0];
    minor = (parts.size() >= 2) ? parts[1] : 0;
    patch = (parts.size() >= 3) ? parts[2] : 0;
    return true;
}

int GetLoginCacheTtlSec() {
    auto& cfg = ConfigMgr::Inst();
    const auto ttl = cfg.GetValue("LoginCache", "TtlSec");
    if (ttl.empty()) {
        return 3600;
    }
    return std::max(60, std::atoi(ttl.c_str()));
}

std::string BuildLoginCacheKey(const std::string& email) {
    return "ulogin_profile_" + email;
}

std::string BuildLoginCacheUidKey(int uid) {
    return "ulogin_profile_uid_" + std::to_string(uid);
}

std::string DecodeLegacyXorPwd(const std::string& input) {
    unsigned int xor_code = static_cast<unsigned int>(input.size() % 255);
    std::string decoded = input;
    for (size_t i = 0; i < decoded.size(); ++i) {
        decoded[i] = static_cast<char>(static_cast<unsigned char>(decoded[i]) ^ xor_code);
    }
    return decoded;
}
} // namespace

namespace gateauthsupport {

const char* MinClientVersion() {
    return kMinClientVersion;
}

int LoginProtocolVersion() {
    return kLoginProtocolVersion;
}

int64_t NowMs() {
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

bool IsQuicRolloutEnabled() {
    return IsTruthyFlag(std::getenv("MEMOCHAT_ENABLE_QUIC"));
}

bool IsClientVersionAllowed(const std::string& clientVer, const std::string& minVer) {
    int cMaj = 0, cMin = 0, cPatch = 0;
    int mMaj = 0, mMin = 0, mPatch = 0;
    if (!ParseSemVer(clientVer, cMaj, cMin, cPatch)) {
        return false;
    }
    if (!ParseSemVer(minVer, mMaj, mMin, mPatch)) {
        return true;
    }

    if (cMaj != mMaj) {
        return cMaj > mMaj;
    }
    if (cMin != mMin) {
        return cMin > mMin;
    }
    return cPatch >= mPatch;
}

std::string GetChatAuthSecret() {
    auto& cfg = ConfigMgr::Inst();
    auto secret = cfg.GetValue("ChatAuth", "HmacSecret");
    if (secret.empty()) {
        secret = "memochat-dev-chat-secret";
    }
    return secret;
}

int GetChatTicketTtlSec() {
    auto& cfg = ConfigMgr::Inst();
    const auto ttl = cfg.GetValue("ChatAuth", "TicketTtlSec");
    if (ttl.empty()) {
        return 20;
    }
    return std::max(5, std::atoi(ttl.c_str()));
}

bool TryLoadCachedLoginProfile(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
    std::string cached_json;
    if (!RedisMgr::GetInstance()->Get(BuildLoginCacheKey(email), cached_json) || cached_json.empty()) {
        return false;
    }

    Json::Value root;
    Json::Reader reader;
    if (!reader.parse(cached_json, root) || !root.isObject()) {
        return false;
    }

    const auto cached_pwd = root.get("pwd", "").asString();
    if (cached_pwd.empty()) {
        return false;
    }
    if (pwd != cached_pwd && DecodeLegacyXorPwd(pwd) != cached_pwd) {
        return false;
    }

    userInfo.pwd = cached_pwd;
    userInfo.name = root.get("name", "").asString();
    userInfo.email = root.get("email", "").asString();
    userInfo.uid = root.get("uid", 0).asInt();
    userInfo.user_id = root.get("user_id", "").asString();
    userInfo.nick = root.get("nick", "").asString();
    userInfo.icon = root.get("icon", "").asString();
    userInfo.desc = root.get("desc", "").asString();
    userInfo.sex = root.get("sex", 0).asInt();
    return userInfo.uid > 0;
}

void CacheLoginProfile(const std::string& email, const UserInfo& userInfo) {
    Json::Value root(Json::objectValue);
    root["uid"] = userInfo.uid;
    root["pwd"] = userInfo.pwd;
    root["name"] = userInfo.name;
    root["email"] = userInfo.email;
    root["user_id"] = userInfo.user_id;
    root["nick"] = userInfo.nick;
    root["icon"] = userInfo.icon;
    root["desc"] = userInfo.desc;
    root["sex"] = userInfo.sex;
    const auto ttl = GetLoginCacheTtlSec();
    RedisMgr::GetInstance()->SetEx(BuildLoginCacheKey(email), root.toStyledString(), ttl);
    if (userInfo.uid > 0) {
        RedisMgr::GetInstance()->SetEx(BuildLoginCacheUidKey(userInfo.uid), email, ttl);
    }
}

void InvalidateLoginCacheByEmail(const std::string& email) {
    if (!email.empty()) {
        RedisMgr::GetInstance()->Del(BuildLoginCacheKey(email));
    }
}

void InvalidateLoginCacheByUid(int uid) {
    if (uid <= 0) {
        return;
    }
    std::string email;
    const auto uidKey = BuildLoginCacheUidKey(uid);
    if (RedisMgr::GetInstance()->Get(uidKey, email) && !email.empty()) {
        RedisMgr::GetInstance()->Del(BuildLoginCacheKey(email));
    }
    RedisMgr::GetInstance()->Del(uidKey);
}

std::vector<ChatRouteNode> LoadGateChatRouteNodes(std::vector<std::string>* load_snapshot,
                                                  std::vector<std::string>* least_loaded_snapshot) {
    if (load_snapshot) {
        load_snapshot->clear();
    }
    if (least_loaded_snapshot) {
        least_loaded_snapshot->clear();
    }
    try {
        auto& cfg = ConfigMgr::Inst();
        const auto cluster = memochat::cluster::LoadChatClusterConfig(
            [&cfg](const std::string& section, const std::string& key) {
                return cfg.GetValue(section, key);
            });
        std::vector<ChatRouteNode> nodes;
        nodes.reserve(cluster.enabledNodes().size());
        int min_online = INT_MAX;
        for (const auto& node : cluster.enabledNodes()) {
            ChatRouteNode route_node;
            route_node.name = node.name;
            route_node.host = node.tcp_host;
            route_node.port = node.tcp_port;
            if (IsQuicRolloutEnabled()) {
                route_node.quic_host = node.quic_host;
                route_node.quic_port = node.quic_port;
            }
            route_node.online_count = 0;
            nodes.push_back(route_node);
            min_online = std::min(min_online, route_node.online_count);
            if (load_snapshot) {
                std::ostringstream one;
                one << node.name << "=" << route_node.online_count;
                load_snapshot->push_back(one.str());
            }
        }
        std::sort(nodes.begin(), nodes.end(), [](const ChatRouteNode& lhs, const ChatRouteNode& rhs) {
            if (lhs.online_count != rhs.online_count) {
                return lhs.online_count < rhs.online_count;
            }
            return lhs.name < rhs.name;
        });
        int priority = 0;
        for (auto& node : nodes) {
            node.priority = priority++;
            if (node.online_count == min_online && least_loaded_snapshot) {
                least_loaded_snapshot->push_back(node.name);
            }
        }
        if (!nodes.empty()) {
            const auto next_index = static_cast<size_t>(g_gate_route_rr_counter.fetch_add(1, std::memory_order_relaxed));
            std::stable_sort(nodes.begin(), nodes.end(), [next_index, min_online](const ChatRouteNode& lhs, const ChatRouteNode& rhs) {
                const bool lhs_least = lhs.online_count == min_online;
                const bool rhs_least = rhs.online_count == min_online;
                if (lhs_least != rhs_least) {
                    return lhs_least > rhs_least;
                }
                if (lhs_least && rhs_least) {
                    return ((lhs.priority + static_cast<int>(next_index)) % 1024) < ((rhs.priority + static_cast<int>(next_index)) % 1024);
                }
                return lhs.priority < rhs.priority;
            });
        }
        return nodes;
    } catch (const std::exception& ex) {
        memolog::LogError("gate.route_select.config_error", "failed to load local chat route config",
            {{"error_type", "cluster_config"}, {"error", ex.what()}});
        if (load_snapshot) {
            load_snapshot->push_back(std::string("config_error:") + ex.what());
        }
        return {};
    }
}

} // namespace gateauthsupport

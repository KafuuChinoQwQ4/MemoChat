#include "AuthLoginSupport.h"
#include "AuthCache.h"
#include "json/GlazeCompat.h"
#include "ConfigMgr.h"
#include "PostgresDao.h"
#include "PostgresMgr.h"
#include "StatusGrpcClient.h"
#include "auth/ChatLoginTicket.h"
#include "cluster/ChatClusterDiscovery.h"
#include "logging/Logger.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <cctype>
#include <sstream>

namespace
{
constexpr const char* kMinClientVersion = "2.0.0";
constexpr int kLoginProtocolVersion = 3;
std::atomic<uint64_t> g_gate_route_rr_counter{0};

bool IsTruthyFlag(const char* raw)
{
    if (raw == nullptr)
    {
        return false;
    }
    const std::string value(raw);
    return value == "1" || value == "true" || value == "TRUE" || value == "on" || value == "ON";
}

bool ParseSemVer(const std::string& ver, int& major, int& minor, int& patch)
{
    major = 0;
    minor = 0;
    patch = 0;
    if (ver.empty())
    {
        return false;
    }
    std::stringstream ss(ver);
    std::string token;
    std::vector<int> parts;
    while (std::getline(ss, token, '.'))
    {
        if (token.empty())
        {
            return false;
        }
        for (char c : token)
        {
            if (!std::isdigit(static_cast<unsigned char>(c)))
            {
                return false;
            }
        }
        parts.push_back(std::stoi(token));
    }
    if (parts.empty() || parts.size() > 3)
    {
        return false;
    }
    major = parts[0];
    minor = (parts.size() >= 2) ? parts[1] : 0;
    patch = (parts.size() >= 3) ? parts[2] : 0;
    return true;
}
} // namespace

namespace gateauthsupport
{

int GetLoginCacheTtlSec()
{
    auto& cfg = ConfigMgr::Inst();
    const auto ttl = cfg.GetValue("LoginCache", "TtlSec");
    if (ttl.empty())
    {
        return 3600;
    }
    return std::max(60, std::atoi(ttl.c_str()));
}

std::string DecodeLegacyXorPwd(const std::string& input)
{
    unsigned int xor_code = static_cast<unsigned int>(input.size() % 255);
    std::string decoded = input;
    for (size_t i = 0; i < decoded.size(); ++i)
    {
        decoded[i] = static_cast<char>(static_cast<unsigned char>(decoded[i]) ^ xor_code);
    }
    return decoded;
}

} // namespace gateauthsupport

namespace gateauthsupport
{

const char* MinClientVersion()
{
    return kMinClientVersion;
}

int LoginProtocolVersion()
{
    return kLoginProtocolVersion;
}

int64_t NowMs()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

bool IsQuicRolloutEnabled()
{
    return IsTruthyFlag(std::getenv("MEMOCHAT_ENABLE_QUIC"));
}

bool IsClientVersionAllowed(const std::string& clientVer, const std::string& minVer)
{
    if (clientVer.empty())
    {
        return true;
    }
    int cMaj = 0, cMin = 0, cPatch = 0;
    int mMaj = 0, mMin = 0, mPatch = 0;
    if (!ParseSemVer(clientVer, cMaj, cMin, cPatch))
    {
        return false;
    }
    if (!ParseSemVer(minVer, mMaj, mMin, mPatch))
    {
        return true;
    }
    if (cMaj != mMaj)
    {
        return cMaj > mMaj;
    }
    if (cMin != mMin)
    {
        return cMin > mMin;
    }
    return cPatch >= mPatch;
}

std::string GetChatAuthSecret()
{
    auto& cfg = ConfigMgr::Inst();
    auto secret = cfg.GetValue("ChatAuth", "HmacSecret");
    if (secret.empty())
    {
        secret = "memochat-dev-chat-secret";
    }
    return secret;
}

int GetChatTicketTtlSec()
{
    auto& cfg = ConfigMgr::Inst();
    const auto ttl = cfg.GetValue("ChatAuth", "TicketTtlSec");
    if (ttl.empty())
    {
        return 20;
    }
    return std::max(5, std::atoi(ttl.c_str()));
}

bool TryLoadCachedLoginProfile(const std::string& email, const std::string& pwd, UserInfo& userInfo)
{
    std::string cached_json;
    if (!memochat::gate::core::AuthCache::Instance().LoadLoginProfileJson(email, cached_json) || cached_json.empty())
    {
        return false;
    }
    memochat::json::JsonValue root;
    if (!memochat::json::reader_parse(cached_json, root) || !memochat::json::glaze_is_object(root))
    {
        return false;
    }
    const auto cached_pwd = memochat::json::glaze_safe_get<std::string>(root["pwd"], "");
    if (cached_pwd.empty())
    {
        return false;
    }
    if (pwd != cached_pwd && DecodeLegacyXorPwd(pwd) != cached_pwd)
    {
        return false;
    }
    userInfo.pwd = cached_pwd;
    userInfo.name = memochat::json::glaze_safe_get<std::string>(root["name"], "");
    userInfo.email = memochat::json::glaze_safe_get<std::string>(root["email"], "");
    userInfo.uid = memochat::json::glaze_safe_get<int64_t>(root["uid"], 0LL);
    userInfo.user_id = memochat::json::glaze_safe_get<std::string>(root["user_id"], "");
    userInfo.nick = memochat::json::glaze_safe_get<std::string>(root["nick"], "");
    userInfo.icon = memochat::json::glaze_safe_get<std::string>(root["icon"], "");
    userInfo.desc = memochat::json::glaze_safe_get<std::string>(root["desc"], "");
    userInfo.sex = memochat::json::glaze_safe_get<int64_t>(root["sex"], 0LL);
    return userInfo.uid > 0;
}

void CacheLoginProfile(const std::string& email, const UserInfo& userInfo)
{
    memochat::json::JsonValue root{};
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
    memochat::gate::core::AuthCache::Instance().StoreLoginProfileJson(email,
                                                                      memochat::json::glaze_stringify(root),
                                                                      ttl);
    if (userInfo.uid > 0)
    {
        memochat::gate::core::AuthCache::Instance().StoreLoginProfileUid(userInfo.uid, email, ttl);
    }
}

bool RefreshLoginProfileFromDb(const std::string& email, UserInfo& userInfo)
{
    if (email.empty() || userInfo.uid <= 0)
    {
        return false;
    }

    ::UserInfo dbUserInfo;
    if (!PostgresMgr::GetInstance()->GetUserInfo(userInfo.uid, dbUserInfo))
    {
        return false;
    }

    userInfo.uid = dbUserInfo.uid;
    userInfo.name = dbUserInfo.name;
    userInfo.email = dbUserInfo.email.empty() ? email : dbUserInfo.email;
    userInfo.user_id = dbUserInfo.user_id;
    userInfo.nick = dbUserInfo.nick;
    userInfo.icon = dbUserInfo.icon;
    userInfo.desc = dbUserInfo.desc;
    userInfo.sex = dbUserInfo.sex;
    CacheLoginProfile(email, userInfo);
    return true;
}

void InvalidateLoginCacheByEmail(const std::string& email)
{
    if (!email.empty())
    {
        memochat::gate::core::AuthCache::Instance().DeleteLoginProfileByEmail(email);
    }
}

void InvalidateLoginCacheByUid(int uid)
{
    if (uid <= 0)
    {
        return;
    }
    std::string email;
    if (memochat::gate::core::AuthCache::Instance().LoadLoginProfileEmailByUid(uid, email) && !email.empty())
    {
        memochat::gate::core::AuthCache::Instance().DeleteLoginProfileByEmail(email);
    }
    memochat::gate::core::AuthCache::Instance().DeleteLoginProfileUid(uid);
}

std::vector<ChatRouteNode> LoadGateChatRouteNodes(std::vector<std::string>* load_snapshot,
                                                  std::vector<std::string>* least_loaded_snapshot)
{
    if (load_snapshot)
    {
        load_snapshot->clear();
    }
    if (least_loaded_snapshot)
    {
        least_loaded_snapshot->clear();
    }
    static const auto kCachedCluster = []()
    {
        auto& cfg = ConfigMgr::Inst();
        return memochat::cluster::LoadChatClusterConfig(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            });
    }();

    try
    {
        std::vector<ChatRouteNode> nodes;
        nodes.reserve(kCachedCluster.enabledNodes().size());
        int min_online = INT_MAX;
        for (const auto& node : kCachedCluster.enabledNodes())
        {
            ChatRouteNode route_node;
            route_node.name = node.name;
            route_node.host = node.tcp_host;
            route_node.port = node.tcp_port;
            if (IsQuicRolloutEnabled())
            {
                route_node.quic_host = node.quic_host;
                route_node.quic_port = node.quic_port;
            }
            route_node.online_count = 0;
            nodes.push_back(route_node);
            min_online = std::min(min_online, route_node.online_count);
            if (load_snapshot)
            {
                std::ostringstream one;
                one << node.name << "=" << route_node.online_count;
                load_snapshot->push_back(one.str());
            }
        }

        std::sort(nodes.begin(),
                  nodes.end(),
                  [](const ChatRouteNode& lhs, const ChatRouteNode& rhs)
                  {
                      if (lhs.online_count != rhs.online_count)
                      {
                          return lhs.online_count < rhs.online_count;
                      }
                      return lhs.name < rhs.name;
                  });

        int priority = 0;
        for (auto& node : nodes)
        {
            node.priority = priority++;
            if (node.online_count == min_online && least_loaded_snapshot)
            {
                least_loaded_snapshot->push_back(node.name);
            }
        }

        if (!nodes.empty())
        {
            const auto next_index =
                static_cast<size_t>(g_gate_route_rr_counter.fetch_add(1, std::memory_order_relaxed));
            const auto least_end = std::find_if(nodes.begin(),
                                                nodes.end(),
                                                [min_online](const ChatRouteNode& node)
                                                {
                                                    return node.online_count != min_online;
                                                });
            const auto least_count = static_cast<size_t>(std::distance(nodes.begin(), least_end));
            if (least_count > 1)
            {
                const auto offset = static_cast<std::vector<ChatRouteNode>::difference_type>(next_index % least_count);
                std::rotate(nodes.begin(), nodes.begin() + offset, least_end);
            }
        }
        return nodes;
    }
    catch (const std::exception& ex)
    {
        memolog::LogError("gate.route_select.config_error",
                          "failed to load local chat route config",
                          {{"error_type", "cluster_config"}, {"error", ex.what()}});
        if (load_snapshot)
        {
            load_snapshot->push_back(std::string("config_error:") + ex.what());
        }
        return {};
    }
}

std::vector<ChatRouteNode> SelectChatRouteViaStatus(int uid, std::string* status_detail, std::string* http_token)
{
    if (status_detail)
    {
        status_detail->clear();
    }
    if (http_token)
    {
        http_token->clear();
    }
    if (uid <= 0)
    {
        if (status_detail)
        {
            *status_detail = "invalid_uid";
        }
        return {};
    }

    try
    {
        const auto rsp = StatusGrpcClient::GetInstance()->GetChatServer(uid);
        if (rsp.error() != ErrorCodes::Success)
        {
            if (status_detail)
            {
                *status_detail = "status_error:" + std::to_string(rsp.error());
            }
            return {};
        }
        if (rsp.server_name().empty() || rsp.host().empty() || rsp.port().empty())
        {
            if (status_detail)
            {
                *status_detail = "incomplete_response";
            }
            return {};
        }

        ChatRouteNode node;
        node.name = rsp.server_name();
        node.host = rsp.host();
        node.port = rsp.port();
        if (IsQuicRolloutEnabled())
        {
            node.quic_host = rsp.quic_host();
            node.quic_port = rsp.quic_port();
        }
        node.online_count = 0;
        node.priority = 0;
        if (http_token)
        {
            *http_token = rsp.token();
        }
        if (status_detail)
        {
            *status_detail = "ok";
        }
        return {node};
    }
    catch (const std::exception& ex)
    {
        if (status_detail)
        {
            *status_detail = std::string("exception:") + ex.what();
        }
        memolog::LogWarn("gate.route_select.status_exception",
                         "StatusServer route selection threw exception",
                         {{"uid", std::to_string(uid)}, {"error", ex.what()}});
        return {};
    }
}

std::vector<ChatRouteNode> SelectChatRouteForLogin(int uid,
                                                   std::vector<std::string>* load_snapshot,
                                                   std::vector<std::string>* least_loaded_snapshot,
                                                   std::string* route_source,
                                                   std::string* status_detail,
                                                   std::string* http_token)
{
    if (route_source)
    {
        route_source->clear();
    }
    auto status_nodes = SelectChatRouteViaStatus(uid, status_detail, http_token);
    if (!status_nodes.empty())
    {
        if (route_source)
        {
            *route_source = "status";
        }
        if (load_snapshot)
        {
            load_snapshot->clear();
            load_snapshot->push_back(status_nodes.front().name + "=selected_by_status");
        }
        if (least_loaded_snapshot)
        {
            least_loaded_snapshot->clear();
            least_loaded_snapshot->push_back(status_nodes.front().name);
        }
        return status_nodes;
    }

    memolog::LogWarn("gate.route_select.status_fallback",
                     "falling back to local chat route selection",
                     {{"uid", std::to_string(uid)}, {"status_detail", status_detail ? *status_detail : ""}});
    if (route_source)
    {
        *route_source = "local_fallback";
    }
    auto local_nodes = LoadGateChatRouteNodes(load_snapshot, least_loaded_snapshot);
    if (local_nodes.empty())
    {
        return {};
    }
    return {local_nodes.front()};
}

} // namespace gateauthsupport

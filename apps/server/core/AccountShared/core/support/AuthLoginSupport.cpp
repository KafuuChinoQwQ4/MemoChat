#include "AuthLoginSupport.hpp"
#include "AuthCache.hpp"
#include "AuthLoginCacheProfileDto.hpp"
#include "json/GlazeCompat.hpp"
#include "ConfigMgr.hpp"
#include "PostgresDao.hpp"
#include "PostgresMgr.hpp"
#include "RedisMgr.hpp"
#include "auth/ChatLoginTicket.hpp"
#include "auth/AuthSecret.hpp"
#include "cluster/ChatClusterDiscovery.hpp"
#include "logging/Logger.hpp"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <mutex>
#include <sstream>

import memochat.account.auth_login_algorithms;

namespace
{
constexpr const char* kMinClientVersion = "2.0.0";
constexpr int kLoginProtocolVersion = 3;
constexpr auto kChatRouteClusterCacheTtl = std::chrono::seconds(60);
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

bool EqualsIgnoreCase(std::string left, std::string right)
{
    std::transform(left.begin(),
                   left.end(),
                   left.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    std::transform(right.begin(),
                   right.end(),
                   right.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    return left == right;
}

memochat::cluster::ChatClusterConfig LoadChatClusterConfigSnapshot()
{
    static std::mutex cache_mutex;
    static memochat::cluster::ChatClusterConfig cached_cluster;
    static std::chrono::steady_clock::time_point cached_at{};
    static bool has_cached_cluster = false;

    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(cache_mutex);
    if (!has_cached_cluster || now - cached_at >= kChatRouteClusterCacheTtl)
    {
        auto& cfg = ConfigMgr::Inst();
        cached_cluster = memochat::cluster::LoadChatClusterConfig(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            });
        cached_at = now;
        has_cached_cluster = true;
    }
    return cached_cluster;
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

int GetRefreshTokenTtlSec()
{
    auto& cfg = ConfigMgr::Inst();
    const auto ttl = cfg.GetValue("AuthRefresh", "RefreshTokenTtlSec");
    if (ttl.empty())
    {
        return 30 * 24 * 60 * 60;
    }
    return std::clamp(std::atoi(ttl.c_str()), 3600, 180 * 24 * 60 * 60);
}

int GetHttpTokenTtlSec()
{
    auto& cfg = ConfigMgr::Inst();
    const auto ttl = cfg.GetValue("AuthToken", "HttpTokenTtlSec");
    if (ttl.empty())
    {
        return 24 * 60 * 60;
    }
    return std::clamp(std::atoi(ttl.c_str()), 60, 30 * 24 * 60 * 60);
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

bool IsWebSocketRolloutEnabled()
{
    return IsTruthyFlag(std::getenv("MEMOCHAT_ENABLE_WS"));
}

bool IsWebTransportRuntimeEnabled()
{
    return IsTruthyFlag(std::getenv("MEMOCHAT_ENABLE_WEBTRANSPORT"));
}

bool IsWebTransportProviderRuntimeEnabled()
{
    return IsTruthyFlag(std::getenv("MEMOCHAT_ENABLE_LWS_WEBTRANSPORT_PROVIDER"));
}

bool IsWebTransportAdvertiseEnabled()
{
    return IsTruthyFlag(std::getenv("MEMOCHAT_ADVERTISE_WEBTRANSPORT"));
}

bool ShouldAdvertiseWebTransportEndpoint(bool config_enabled)
{
    return (config_enabled || IsWebTransportRuntimeEnabled()) && IsWebTransportProviderRuntimeEnabled() &&
           IsWebTransportAdvertiseEnabled();
}

bool IsClientVersionAllowed(const std::string& clientVer, const std::string& minVer)
{
    if (clientVer.empty())
    {
        return false;
    }
    memochat::account::auth::modules::SemVerParts client_version;
    memochat::account::auth::modules::SemVerParts min_version;
    if (!memochat::account::auth::modules::ParseSemVer(clientVer.data(), clientVer.size(), client_version))
    {
        return false;
    }
    if (!memochat::account::auth::modules::ParseSemVer(minVer.data(), minVer.size(), min_version))
    {
        return true;
    }
    return memochat::account::auth::modules::CompareSemVer(client_version, min_version) >= 0;
}

std::string GetChatAuthSecret()
{
    auto& cfg = ConfigMgr::Inst();
    auto secret = cfg.GetValue("ChatAuth", "HmacSecret");
    if (secret.empty())
    {
        secret = "memochat-dev-chat-secret";
    }
    if (memochat::auth::IsWellKnownDevHmacSecret(secret))
    {
        static std::once_flag dev_secret_warned;
        std::call_once(dev_secret_warned,
                       []()
                       {
                           memolog::LogWarn("auth.hmac_secret.dev_default",
                                            "ChatAuth HmacSecret is the well-known dev default; set env "
                                            "MEMOCHAT_CHATAUTH_HMACSECRET in production");
                       });
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
    return std::clamp(std::atoi(ttl.c_str()), 5, 300);
}

bool TryLoadCachedLoginProfile(const std::string& email, UserInfo& userInfo)
{
    std::string cached_json;
    if (!memochat::gate::core::AuthCache::Instance().LoadLoginProfileJson(email, cached_json) || cached_json.empty())
    {
        return false;
    }
    UserInfo cached_user;
    if (!DecodeLoginCacheProfile(cached_json, &cached_user))
    {
        return false;
    }
    userInfo = cached_user;
    return userInfo.uid > 0;
}

void CacheLoginProfile(const std::string& email, const UserInfo& userInfo)
{
    std::string cache_body;
    if (!EncodeLoginCacheProfile(userInfo, &cache_body))
    {
        return;
    }
    const auto ttl = GetLoginCacheTtlSec();
    memochat::gate::core::AuthCache::Instance().StoreLoginProfileJson(email, cache_body, ttl);
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
    if (!dbUserInfo.email.empty() && !EqualsIgnoreCase(dbUserInfo.email, email))
    {
        memolog::LogWarn(
            "gate.login_cache.email_mismatch",
            "refusing to refresh login profile with mismatched email",
            {{"requested_email", email}, {"db_email", dbUserInfo.email}, {"uid", std::to_string(userInfo.uid)}});
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
    const auto cluster = LoadChatClusterConfigSnapshot();

    try
    {
        std::vector<ChatRouteNode> nodes;
        nodes.reserve(cluster.enabledNodes().size());
        int min_online = INT_MAX;
        for (const auto& node : cluster.enabledNodes())
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
            route_node.ws_enabled = node.ws_enabled || IsWebSocketRolloutEnabled();
            route_node.ws_host = node.ws_host;
            route_node.ws_port = node.ws_port;
            route_node.ws_path = node.ws_path;
            route_node.ws_tls = node.ws_tls;
            route_node.wt_enabled = ShouldAdvertiseWebTransportEndpoint(node.wt_enabled);
            route_node.wt_host = node.wt_host;
            route_node.wt_port = node.wt_port;
            route_node.wt_path = node.wt_path;
            route_node.wt_tls = node.wt_tls;
            const std::string online_users_key = std::string(SERVER_ONLINE_USERS_PREFIX) + node.name;
            int online_count = 0;
            const bool load_ok = RedisMgr::GetInstance()->SCard(online_users_key, online_count);
            if (!load_ok || online_count < 0)
            {
                online_count = 0;
            }
            route_node.online_count = online_count;
            nodes.push_back(route_node);
            min_online = std::min(min_online, route_node.online_count);
            if (load_snapshot)
            {
                std::ostringstream one;
                one << node.name << "=" << route_node.online_count;
                if (!load_ok)
                {
                    one << "(redis-fallback)";
                }
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

std::vector<ChatRouteNode> SelectChatRouteForLogin(int uid,
                                                   std::vector<std::string>* load_snapshot,
                                                   std::vector<std::string>* least_loaded_snapshot,
                                                   std::string* route_source,
                                                   std::string* status_detail,
                                                   std::string* http_token)
{
    (void) uid;
    if (route_source)
    {
        *route_source = "local";
    }
    if (status_detail)
    {
        status_detail->clear();
    }
    // Chat-server selection runs in-process: GateServer reads per-server online
    // counts from Redis (least-connections) and signs the login ticket for the
    // chosen target. k8s owns coarse failover/replica scaling for the chat tier;
    // the per-login target decision stays here because chat sessions are stateful
    // (the signed ticket binds a uid to a specific target_server).
    //
    // The HTTP token is left empty so the caller rotates it via AuthCache on the
    // shared `utoken_<uid>` key (the same key focused services validate against).
    if (http_token)
    {
        http_token->clear();
    }
    auto local_nodes = LoadGateChatRouteNodes(load_snapshot, least_loaded_snapshot);
    if (local_nodes.empty())
    {
        if (status_detail)
        {
            *status_detail = "no_chat_server";
        }
        return {};
    }
    if (status_detail)
    {
        *status_detail = "ok";
    }
    return {local_nodes.front()};
}

} // namespace gateauthsupport

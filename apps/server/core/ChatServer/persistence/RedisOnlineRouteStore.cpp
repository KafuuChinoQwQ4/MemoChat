#include "RedisOnlineRouteStore.h"

#include "CSession.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "cluster/ChatClusterDiscovery.h"
#include "const.h"

#include <algorithm>
#include <vector>

namespace
{
std::string TrimCopyRouteStore(const std::string& text)
{
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos)
    {
        return std::string();
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::vector<std::string> KnownChatServerNamesRouteStore()
{
    std::vector<std::string> servers;
    auto& cfg = ConfigMgr::Inst();
    const auto cluster = memochat::cluster::LoadChatClusterConfig(
        [&cfg](const std::string& section, const std::string& key)
        {
            return cfg.GetValue(section, key);
        },
        std::string());
    for (const auto& node : cluster.enabledNodes())
    {
        servers.push_back(node.name);
    }
    return servers;
}
} // namespace

std::string RedisOnlineRouteStore::SelfServerName() const
{
    return ConfigMgr::Inst()["SelfServer"]["Name"];
}

void RedisOnlineRouteStore::RepairOnlineRoute(int uid, const std::shared_ptr<CSession>& session)
{
    if (uid <= 0 || !session)
    {
        return;
    }
    const auto server_name = SelfServerName();
    if (server_name.empty())
    {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RedisMgr::GetInstance()->Set(USERIPPREFIX + uid_str, server_name);
    RedisMgr::GetInstance()->Set(USER_SESSION_PREFIX + uid_str, session->GetSessionId());
    RedisMgr::GetInstance()->SAdd(ServerOnlineUsersKey(server_name), uid_str);
}

std::string RedisOnlineRouteStore::FindUserServer(int uid)
{
    if (uid <= 0)
    {
        return std::string();
    }
    std::string server_name;
    RedisMgr::GetInstance()->Get(USERIPPREFIX + std::to_string(uid), server_name);
    return server_name;
}

std::string RedisOnlineRouteStore::ResolveServerFromOnlineSets(int uid)
{
    if (uid <= 0)
    {
        return std::string();
    }

    const auto uid_str = std::to_string(uid);
    for (const auto& server_name : KnownChatServerNamesRouteStore())
    {
        std::vector<std::string> online_uids;
        RedisMgr::GetInstance()->SMembers(ServerOnlineUsersKey(server_name), online_uids);
        if (std::find(online_uids.begin(), online_uids.end(), uid_str) != online_uids.end())
        {
            return server_name;
        }
    }
    return std::string();
}

void RedisOnlineRouteStore::SetUserServer(int uid, const std::string& server_name)
{
    if (uid <= 0 || server_name.empty())
    {
        return;
    }
    RedisMgr::GetInstance()->Set(USERIPPREFIX + std::to_string(uid), server_name);
}

void RedisOnlineRouteStore::ClearTrackedOnlineRoute(int uid, const std::string& server_name)
{
    if (uid <= 0 || server_name.empty())
    {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
    RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid_str);
    RedisMgr::GetInstance()->SRem(ServerOnlineUsersKey(server_name), uid_str);
}

std::string RedisOnlineRouteStore::ServerOnlineUsersKey(const std::string& server_name) const
{
    return std::string(SERVER_ONLINE_USERS_PREFIX) + server_name;
}

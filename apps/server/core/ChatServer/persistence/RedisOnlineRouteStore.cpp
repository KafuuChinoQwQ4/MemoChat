#include "RedisOnlineRouteStore.hpp"

#include "ConfigMgr.hpp"
#include "chat_lua_scripts.hpp"
#include "OnlineRouteSessionSupport.hpp"
#include "RedisMgr.hpp"
#include "cluster/ChatClusterDiscovery.hpp"
#include "const.hpp"

import memochat.chat.online_route_store_algorithms;

#include <algorithm>
#include <vector>

namespace
{
namespace online_route_modules = memochat::chat::persistence::online_route_store::modules;

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
    if (!online_route_modules::ShouldCheckRepairSession(uid, session != nullptr))
    {
        return;
    }
    const auto server_name = SelfServerName();
    if (!online_route_modules::HasUsableServerName(server_name.empty()))
    {
        return;
    }
    const auto uid_str = std::to_string(uid);
    RepairOnlineRouteAtomic(uid_str, server_name, ExtractOnlineRouteSessionId(session));
}

bool RedisOnlineRouteStore::RepairOnlineRouteAtomic(const std::string& uid_str,
                                                    const std::string& server_name,
                                                    const std::string& session_id)
{
    const std::string kScript(memochat::chat::lua_scripts::krepair_online_route);

    const auto key1 = std::string(USERIPPREFIX) + uid_str;
    const auto key2 = std::string(USER_SESSION_PREFIX) + uid_str;
    const auto key3 = ServerOnlineUsersKey(server_name);

    auto* ctx = RedisMgr::GetInstance()->getRawConnection();
    if (ctx == nullptr)
    {
        return false;
    }

    auto release = [&]()
    {
        RedisMgr::GetInstance()->returnConnection(ctx);
    };

    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx,
                                                              "EVAL %s 3 %s %s %s %s %s %s",
                                                              kScript.c_str(),
                                                              key1.c_str(),
                                                              key2.c_str(),
                                                              key3.c_str(),
                                                              server_name.c_str(),
                                                              session_id.c_str(),
                                                              uid_str.c_str()));

    if (reply == nullptr)
    {
        release();
        return false;
    }
    freeReplyObject(reply);
    release();
    return true;
}

std::string RedisOnlineRouteStore::FindUserServer(int uid)
{
    if (!online_route_modules::ShouldReadUserRoute(uid))
    {
        return std::string();
    }
    std::string server_name;
    RedisMgr::GetInstance()->Get(USERIPPREFIX + std::to_string(uid), server_name);
    return server_name;
}

std::string RedisOnlineRouteStore::ResolveServerFromOnlineSets(int uid)
{
    if (!online_route_modules::ShouldSearchOnlineSets(uid))
    {
        return std::string();
    }

    const auto uid_str = std::to_string(uid);
    for (const auto& server_name : KnownChatServerNamesRouteStore())
    {
        std::vector<std::string> online_uids;
        RedisMgr::GetInstance()->SMembers(ServerOnlineUsersKey(server_name), online_uids);
        if (online_route_modules::ShouldUseOnlineSetHit(std::find(online_uids.begin(), online_uids.end(), uid_str) !=
                                                        online_uids.end()))
        {
            return server_name;
        }
    }
    return std::string();
}

void RedisOnlineRouteStore::SetUserServer(int uid, const std::string& server_name)
{
    if (!online_route_modules::ShouldWriteUserRoute(uid, server_name.empty()))
    {
        return;
    }
    RedisMgr::GetInstance()->Set(USERIPPREFIX + std::to_string(uid), server_name);
}

void RedisOnlineRouteStore::ClearTrackedOnlineRoute(int uid, const std::string& server_name)
{
    if (!online_route_modules::ShouldClearTrackedRoute(uid, server_name.empty()))
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

#pragma once

#include "ports/IOnlineRouteStore.hpp"

class RedisOnlineRouteStore : public IOnlineRouteStore
{
public:
    std::string SelfServerName() const override;
    void RepairOnlineRoute(int uid, const std::shared_ptr<IChatSession>& session) override;
    std::string FindUserServer(int uid) override;
    std::string ResolveServerFromOnlineSets(int uid) override;
    void SetUserServer(int uid, const std::string& server_name) override;
    void ClearTrackedOnlineRoute(int uid, const std::string& server_name) override;

private:
    std::string ServerOnlineUsersKey(const std::string& server_name) const;
    bool
    RepairOnlineRouteAtomic(const std::string& uid_str, const std::string& server_name, const std::string& session_id);
};

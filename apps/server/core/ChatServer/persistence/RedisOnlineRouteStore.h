#pragma once

#include "ports/IOnlineRouteStore.h"

class RedisOnlineRouteStore : public IOnlineRouteStore
{
public:
    std::string SelfServerName() const override;
    void RepairOnlineRoute(int uid, const std::shared_ptr<CSession>& session) override;
    std::string FindUserServer(int uid) override;
    std::string ResolveServerFromOnlineSets(int uid) override;
    void SetUserServer(int uid, const std::string& server_name) override;
    void ClearTrackedOnlineRoute(int uid, const std::string& server_name) override;

private:
    std::string ServerOnlineUsersKey(const std::string& server_name) const;
};

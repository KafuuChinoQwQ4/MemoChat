#pragma once

#include <memory>
#include <string>

class IChatSession;

class IOnlineRouteStore
{
public:
    virtual ~IOnlineRouteStore() = default;

    virtual std::string SelfServerName() const = 0;
    virtual void RepairOnlineRoute(int uid, const std::shared_ptr<IChatSession>& session) = 0;
    virtual std::string FindUserServer(int uid) = 0;
    virtual std::string ResolveServerFromOnlineSets(int uid) = 0;
    virtual void SetUserServer(int uid, const std::string& server_name) = 0;
    virtual void ClearTrackedOnlineRoute(int uid, const std::string& server_name) = 0;
};

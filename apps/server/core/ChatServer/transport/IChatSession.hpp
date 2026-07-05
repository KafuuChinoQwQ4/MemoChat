#pragma once

#include <ctime>
#include <string>

class IChatSession
{
public:
    virtual ~IChatSession() = default;

    virtual std::string sessionId() const = 0;
    virtual int userId() const = 0;
    virtual void setUserId(int uid) = 0;
    virtual std::string transportName() const = 0;
    virtual void send(std::string payload, short msg_id) = 0;
    virtual void close() = 0;
    virtual bool isHeartbeatExpired(std::time_t now) const = 0;
    virtual void updateHeartbeat() = 0;
    virtual bool tryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds) = 0;
    virtual void markOnlineRouteRefreshed(std::time_t now) = 0;
};

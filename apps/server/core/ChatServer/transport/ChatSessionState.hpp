#pragma once

#include <atomic>
#include <ctime>
#include <string>

#include "random/Uuid.hpp"

namespace memochat::chatserver
{

class ChatSessionState
{
public:
    ChatSessionState()
    {
        memochat::random::GenerateUuid(_session_id, &_startup_error);
        _last_heartbeat.store(std::time(nullptr), std::memory_order_relaxed);
        _last_online_route_refresh.store(0, std::memory_order_relaxed);
    }

    bool Ready() const noexcept
    {
        return _startup_error.empty();
    }

    const std::string& startupError() const noexcept
    {
        return _startup_error;
    }

    const std::string& sessionIdRef() const
    {
        return _session_id;
    }

    std::string sessionId() const
    {
        return _session_id;
    }

    int userId() const
    {
        return _user_uid.load(std::memory_order_relaxed);
    }

    void setUserId(int uid)
    {
        _user_uid.store(uid, std::memory_order_relaxed);
    }

    bool isHeartbeatExpired(std::time_t now) const
    {
        constexpr double kHeartbeatExpireSeconds = 45.0;
        return std::difftime(now, _last_heartbeat.load(std::memory_order_relaxed)) > kHeartbeatExpireSeconds;
    }

    void updateHeartbeat()
    {
        _last_heartbeat.store(std::time(nullptr), std::memory_order_relaxed);
    }

    bool tryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds)
    {
        if (interval_seconds <= 0)
        {
            _last_online_route_refresh.store(now, std::memory_order_relaxed);
            return true;
        }

        auto last = _last_online_route_refresh.load(std::memory_order_relaxed);
        while (last <= 0 || std::difftime(now, last) >= interval_seconds)
        {
            if (_last_online_route_refresh.compare_exchange_weak(last,
                                                                 now,
                                                                 std::memory_order_relaxed,
                                                                 std::memory_order_relaxed))
            {
                return true;
            }
        }
        return false;
    }

    void markOnlineRouteRefreshed(std::time_t now)
    {
        _last_online_route_refresh.store(now, std::memory_order_relaxed);
    }

private:
    std::string _session_id;
    std::string _startup_error;
    std::atomic<int> _user_uid{0};
    std::atomic<std::time_t> _last_heartbeat{0};
    std::atomic<std::time_t> _last_online_route_refresh{0};
};

} // namespace memochat::chatserver

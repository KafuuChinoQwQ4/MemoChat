#include "CSession.hpp"

#include "ChatSessionCleanupSupport.hpp"
#include "json/GlazeCompat.hpp"
#include "random/Uuid.hpp"

#include <iostream>
#include <utility>

namespace
{
std::string JsonToWireString(const memochat::json::JsonValue& v)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, v);
}
} // namespace

CSession::CSession(boost::asio::io_context& io_context, IChatSessionHost* host)
    : CSession(host)
{
    (void) io_context;
}

CSession::CSession(IChatSessionHost* host)
    : _server(host)
    , _user_uid(0)
{
    memochat::random::GenerateUuid(_session_id, &_startup_error);
    _last_heartbeat = std::time(nullptr);
    _last_online_route_refresh = 0;
}

CSession::~CSession()
{
    std::cout << "~CSession destruct" << std::endl;
}

std::string& CSession::GetSessionId()
{
    return _session_id;
}

bool CSession::Ready() const noexcept
{
    return _startup_error.empty();
}

const std::string& CSession::startupError() const noexcept
{
    return _startup_error;
}

void CSession::SetUserId(int uid)
{
    _user_uid = uid;
}

int CSession::GetUserId()
{
    return _user_uid;
}

void CSession::Start()
{
}

void CSession::Send(std::string msg, short msgid)
{
    (void) msg;
    (void) msgid;
}

void CSession::Send(char* msg, short max_length, short msgid)
{
    if (msg == nullptr || max_length <= 0)
    {
        return;
    }
    Send(std::string(msg, msg + max_length), msgid);
}

void CSession::Close()
{
}

void CSession::DetachServer()
{
    _server.store(nullptr, std::memory_order_release);
}

std::shared_ptr<CSession> CSession::SharedSelf()
{
    return shared_from_this();
}

void CSession::NotifyOffline(int uid)
{
    memochat::json::JsonValue rtvalue;
    rtvalue["error"] = ErrorCodes::Success;
    rtvalue["uid"] = uid;

    Send(JsonToWireString(rtvalue), ID_NOTIFY_OFF_LINE_REQ);
}

bool CSession::IsHeartbeatExpired(std::time_t now)
{
    return isHeartbeatExpired(now);
}

bool CSession::isHeartbeatExpired(std::time_t now) const
{
    constexpr double kHeartbeatExpireSeconds = 45.0;
    double diff_sec = std::difftime(now, _last_heartbeat);
    if (diff_sec > kHeartbeatExpireSeconds)
    {
        std::cout << "heartbeat expired, session id is  " << _session_id << std::endl;
        return true;
    }

    return false;
}

void CSession::UpdateHeartbeat()
{
    updateHeartbeat();
}

void CSession::updateHeartbeat()
{
    time_t now = std::time(nullptr);
    _last_heartbeat = now;
}

bool CSession::tryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds)
{
    return TryMarkOnlineRouteRefreshDue(now, interval_seconds);
}

void CSession::markOnlineRouteRefreshed(std::time_t now)
{
    MarkOnlineRouteRefreshed(now);
}

bool CSession::TryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds)
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

void CSession::MarkOnlineRouteRefreshed(std::time_t now)
{
    _last_online_route_refresh.store(now, std::memory_order_relaxed);
}

std::string CSession::sessionId() const
{
    return _session_id;
}

int CSession::userId() const
{
    return _user_uid;
}

void CSession::setUserId(int uid)
{
    SetUserId(uid);
}

std::string CSession::transportName() const
{
    return "session";
}

void CSession::send(std::string payload, short msg_id)
{
    Send(std::move(payload), msg_id);
}

void CSession::close()
{
    Close();
}

IChatSessionHost* CSession::Server() const
{
    return _server.load(std::memory_order_acquire);
}

void CSession::DealExceptionSession()
{
    memochat::chatserver::CleanupExceptionSession(std::static_pointer_cast<IChatSession>(shared_from_this()),
                                                  _server.load(std::memory_order_acquire));
}

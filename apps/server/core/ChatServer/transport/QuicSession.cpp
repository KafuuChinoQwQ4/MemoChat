#include "QuicSession.hpp"

#include "ChatSessionCleanupSupport.hpp"
#include "ChatFrameDispatch.hpp"

#include <iostream>

QuicSession::QuicSession(boost::asio::io_context& io_context, SendCallback send_callback, CloseCallback close_callback)
    : _send_callback(std::move(send_callback))
    , _close_callback(std::move(close_callback))
{
    (void) io_context;
}

QuicSession::~QuicSession() = default;

bool QuicSession::Ready() const noexcept
{
    return _state.Ready();
}

const std::string& QuicSession::startupError() const noexcept
{
    return _state.startupError();
}

void QuicSession::Start()
{
}

void QuicSession::Send(std::string msg, short msgid)
{
    if (_closed)
    {
        return;
    }
    if (!_send_callback || !_send_callback(msg, msgid))
    {
        std::cout << "quic session send failed, session id is " << sessionId() << std::endl;
        Close();
    }
}

void QuicSession::Send(char* msg, short max_length, short msgid)
{
    if (msg == nullptr || max_length <= 0)
    {
        return;
    }
    Send(std::string(msg, msg + max_length), msgid);
}

void QuicSession::Close()
{
    if (_closed)
    {
        return;
    }
    _closed = true;
    if (_close_callback)
    {
        _close_callback(sessionId());
    }
    if (userId() > 0)
    {
        if (auto self = weak_from_this().lock())
        {
            memochat::chatserver::CleanupExceptionSession(std::static_pointer_cast<IChatSession>(self));
        }
    }
}

std::string QuicSession::transportName() const
{
    return "quic";
}

void QuicSession::HandleInboundMessage(short msgid, const std::string& payload)
{
    memochat::chatserver::transport::PostInboundChatFrame(std::static_pointer_cast<IChatSession>(shared_from_this()),
                                                          msgid,
                                                          payload);
}

std::string QuicSession::sessionId() const
{
    return _state.sessionId();
}

int QuicSession::userId() const
{
    return _state.userId();
}

void QuicSession::setUserId(int uid)
{
    _state.setUserId(uid);
}

void QuicSession::send(std::string payload, short msg_id)
{
    Send(std::move(payload), msg_id);
}

void QuicSession::close()
{
    Close();
}

bool QuicSession::isHeartbeatExpired(std::time_t now) const
{
    return _state.isHeartbeatExpired(now);
}

void QuicSession::updateHeartbeat()
{
    _state.updateHeartbeat();
}

bool QuicSession::tryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds)
{
    return _state.tryMarkOnlineRouteRefreshDue(now, interval_seconds);
}

void QuicSession::markOnlineRouteRefreshed(std::time_t now)
{
    _state.markOnlineRouteRefreshed(now);
}

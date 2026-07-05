#include "WebTransportSession.hpp"

#include "ChatSessionCleanupSupport.hpp"
#include "ChatFrameCodec.hpp"
#include "ChatFrameDispatch.hpp"
#include "logging/Logger.hpp"

#include <cstdlib>
#include <iostream>
#include <string_view>
#include <utility>

namespace memochat::chatserver
{

namespace
{
bool TraceWebTransportSessionEnabled()
{
    const char* value = std::getenv("MEMOCHAT_TRACE_WEBTRANSPORT_PROVIDER");
    return value != nullptr && value[0] != '\0' && std::string_view(value) != "0";
}

void TraceWebTransportSessionEvent(std::string_view event,
                                   std::string_view session_id,
                                   short msg_id = 0,
                                   std::size_t length = 0)
{
    if (!TraceWebTransportSessionEnabled())
    {
        return;
    }

    std::cerr << "[webtransport.session] event=" << event;
    if (!session_id.empty())
    {
        std::cerr << " session_id=" << session_id;
    }
    if (msg_id != 0)
    {
        std::cerr << " msg_id=" << msg_id;
    }
    if (length != 0)
    {
        std::cerr << " len=" << length;
    }
    std::cerr << '\n';
}
} // namespace

WebTransportSession::WebTransportSession(SendFrameCallback send_callback,
                                         CloseCallback close_callback,
                                         InboundFrameCallback inbound_callback)
    : _send_callback(std::move(send_callback))
    , _close_callback(std::move(close_callback))
    , _inbound_callback(std::move(inbound_callback))
{
    if (!_inbound_callback)
    {
        _inbound_callback = [](const std::shared_ptr<IChatSession>& session, short msg_id, std::string_view payload)
        {
            return transport::PostInboundChatFrame(session, msg_id, payload);
        };
    }
}

WebTransportSession::~WebTransportSession() = default;

std::shared_ptr<WebTransportSession> WebTransportSession::Self()
{
    return shared_from_this();
}

void WebTransportSession::Start()
{
}

void WebTransportSession::Send(std::string msg, short msgid)
{
    if (_closed.load(std::memory_order_acquire))
    {
        return;
    }

    auto encoded = transport::ChatFrameCodec::Encode(msgid, msg);
    TraceWebTransportSessionEvent("send", sessionId(), msgid, msg.size());
    if (!encoded)
    {
        memolog::LogWarn("webtransport.session.send_invalid_frame",
                         "WebTransport chat frame encode rejected payload",
                         {{"session_id", sessionId()}, {"msg_id", std::to_string(msgid)}});
        return;
    }

    if (!_send_callback || !_send_callback(std::move(*encoded)))
    {
        TraceWebTransportSessionEvent("send_failed", sessionId(), msgid);
        memolog::LogWarn("webtransport.session.send_failed",
                         "WebTransport provider rejected encoded chat frame",
                         {{"session_id", sessionId()}, {"msg_id", std::to_string(msgid)}});
        Close();
        return;
    }
    TraceWebTransportSessionEvent("send_queued", sessionId(), msgid);
}

void WebTransportSession::Send(char* msg, short max_length, short msgid)
{
    if (msg == nullptr || max_length <= 0)
    {
        return;
    }
    Send(std::string(msg, msg + max_length), msgid);
}

bool WebTransportSession::AcceptStreamBytes(std::string_view bytes)
{
    if (_closed.load(std::memory_order_acquire))
    {
        return false;
    }

    if (bytes.size() > kMaxDecodeBufferBytes || _decode_buffer.size() > kMaxDecodeBufferBytes - bytes.size())
    {
        memolog::LogWarn("webtransport.session.decode_buffer_too_large",
                         "WebTransport chat frame decode buffer exceeded limit",
                         {{"session_id", sessionId()},
                          {"buffer_size", std::to_string(_decode_buffer.size())},
                          {"incoming_size", std::to_string(bytes.size())},
                          {"limit", std::to_string(kMaxDecodeBufferBytes)}});
        Close();
        return false;
    }

    _decode_buffer.insert(_decode_buffer.end(), bytes.begin(), bytes.end());

    for (;;)
    {
        transport::ChatFrame frame;
        const auto status = transport::ChatFrameCodec::DecodeOne(_decode_buffer, frame);
        if (status == transport::ChatFrameDecodeStatus::NeedMoreData)
        {
            return true;
        }
        if (status == transport::ChatFrameDecodeStatus::Invalid)
        {
            memolog::LogWarn("webtransport.session.invalid_frame",
                             "WebTransport chat frame decode failed",
                             {{"session_id", sessionId()}});
            Close();
            return false;
        }

        TraceWebTransportSessionEvent("decode", sessionId(), frame.msg_id, frame.payload.size());
        if (!_inbound_callback(Self(), frame.msg_id, frame.payload))
        {
            TraceWebTransportSessionEvent("dispatch_failed", sessionId(), frame.msg_id);
            memolog::LogWarn("webtransport.session.dispatch_rejected",
                             "WebTransport inbound chat frame dispatch failed",
                             {{"session_id", sessionId()}, {"msg_id", std::to_string(frame.msg_id)}});
            Close();
            return false;
        }
        TraceWebTransportSessionEvent("dispatch_queued", sessionId(), frame.msg_id);
    }
}

void WebTransportSession::Close()
{
    if (_closed.exchange(true, std::memory_order_acq_rel))
    {
        return;
    }

    if (!_close_notified.exchange(true, std::memory_order_acq_rel) && _close_callback)
    {
        _close_callback(sessionId());
    }

    if (userId() > 0)
    {
        CleanupExceptionSession(Self());
    }
}

std::string WebTransportSession::transportName() const
{
    return "webtransport";
}

std::string WebTransportSession::sessionId() const
{
    return _state.sessionId();
}

int WebTransportSession::userId() const
{
    return _state.userId();
}

void WebTransportSession::setUserId(int uid)
{
    _state.setUserId(uid);
}

void WebTransportSession::send(std::string payload, short msg_id)
{
    Send(std::move(payload), msg_id);
}

void WebTransportSession::close()
{
    Close();
}

bool WebTransportSession::acceptStreamBytes(std::string_view bytes)
{
    return AcceptStreamBytes(bytes);
}

bool WebTransportSession::isClosed() const
{
    return _closed.load(std::memory_order_acquire);
}

bool WebTransportSession::isHeartbeatExpired(std::time_t now) const
{
    return _state.isHeartbeatExpired(now);
}

void WebTransportSession::updateHeartbeat()
{
    _state.updateHeartbeat();
}

bool WebTransportSession::tryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds)
{
    return _state.tryMarkOnlineRouteRefreshDue(now, interval_seconds);
}

void WebTransportSession::markOnlineRouteRefreshed(std::time_t now)
{
    _state.markOnlineRouteRefreshed(now);
}

} // namespace memochat::chatserver

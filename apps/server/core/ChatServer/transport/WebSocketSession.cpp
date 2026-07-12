#include "WebSocketSession.hpp"

#include "ChatSessionCleanupSupport.hpp"
#include "ChatFrameCodec.hpp"
#include "ChatFrameDispatch.hpp"
#include "logging/Logger.hpp"

#include <boost/beast/core/buffers_to_string.hpp>

#include <cstdlib>
#include <limits>
#include <utility>

namespace memochat::chatserver
{

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = net::ip::tcp;

WebSocketSession::WebSocketSession(tcp::socket&& socket,
                                   std::string path,
                                   CloseCallback close_callback,
                                   InboundFrameCallback inbound_callback)
    : _ws(std::move(socket))
    , _path(std::move(path))
    , _close_callback(std::move(close_callback))
    , _inbound_callback(std::move(inbound_callback))
{
    if (_path.empty())
    {
        _path = "/ws";
    }
    if (!_inbound_callback)
    {
        _inbound_callback = [](const std::shared_ptr<IChatSession>& session, short msg_id, std::string_view payload)
        {
            return transport::PostInboundChatFrame(session, msg_id, payload);
        };
    }
}

WebSocketSession::~WebSocketSession() = default;

bool WebSocketSession::Ready() const noexcept
{
    return _state.Ready();
}

const std::string& WebSocketSession::startupError() const noexcept
{
    return _state.startupError();
}

std::shared_ptr<WebSocketSession> WebSocketSession::Self()
{
    return shared_from_this();
}

void WebSocketSession::Start()
{
    _ws.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    _ws.set_option(websocket::stream_base::decorator(
        [](websocket::response_type& response)
        {
            response.set(http::field::server, "MemoChat ChatServer WebSocket");
        }));

    auto self = Self();
    http::async_read(_ws.next_layer(),
                     _buffer,
                     _request,
                     [self](beast::error_code ec, std::size_t)
                     {
                         self->onHttpRead(ec);
                     });
}

void WebSocketSession::Send(std::string msg, short msgid)
{
    if (_closed.load(std::memory_order_acquire))
    {
        return;
    }
    auto encoded = transport::ChatFrameCodec::Encode(msgid, msg);
    if (!encoded)
    {
        memolog::LogWarn("websocket.session.send_invalid_frame",
                         "WebSocket chat frame encode rejected payload",
                         {{"session_id", sessionId()}, {"msg_id", std::to_string(msgid)}});
        return;
    }

    auto self = Self();
    net::post(_ws.get_executor(),
              [self, frame = std::move(*encoded)]() mutable
              {
                  self->enqueueSend(std::move(frame));
              });
}

void WebSocketSession::Send(char* msg, short max_length, short msgid)
{
    if (msg == nullptr || max_length <= 0)
    {
        return;
    }
    Send(std::string(msg, msg + max_length), msgid);
}

void WebSocketSession::Close()
{
    auto self = Self();
    net::post(_ws.get_executor(),
              [self]()
              {
                  self->closeOnExecutor();
              });
}

std::string WebSocketSession::transportName() const
{
    return "websocket";
}

std::string WebSocketSession::sessionId() const
{
    return _state.sessionId();
}

int WebSocketSession::userId() const
{
    return _state.userId();
}

void WebSocketSession::setUserId(int uid)
{
    _state.setUserId(uid);
}

void WebSocketSession::send(std::string payload, short msg_id)
{
    Send(std::move(payload), msg_id);
}

void WebSocketSession::close()
{
    Close();
}

bool WebSocketSession::isHeartbeatExpired(std::time_t now) const
{
    return _state.isHeartbeatExpired(now);
}

void WebSocketSession::updateHeartbeat()
{
    _state.updateHeartbeat();
}

bool WebSocketSession::tryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds)
{
    return _state.tryMarkOnlineRouteRefreshDue(now, interval_seconds);
}

void WebSocketSession::markOnlineRouteRefreshed(std::time_t now)
{
    _state.markOnlineRouteRefreshed(now);
}

bool WebSocketSession::pathMatches() const
{
    const auto target = _request.target();
    const auto query_pos = target.find('?');
    const auto path = query_pos == boost::beast::string_view::npos ? target : target.substr(0, query_pos);
    return path == _path;
}

void WebSocketSession::onHttpRead(beast::error_code ec)
{
    if (ec)
    {
        finishClosed();
        return;
    }

    if (!pathMatches())
    {
        memolog::LogWarn(
            "websocket.session.bad_path",
            "rejected WebSocket chat connection with invalid path",
            {{"session_id", sessionId()}, {"target", std::string(_request.target())}, {"expected_path", _path}});
        closeOnExecutor();
        return;
    }

    _ws.binary(true);
    auto self = Self();
    _ws.async_accept(_request,
                     [self](beast::error_code accept_ec)
                     {
                         self->onAccept(accept_ec);
                     });
}

void WebSocketSession::onAccept(beast::error_code ec)
{
    if (ec)
    {
        finishClosed();
        return;
    }

    memolog::LogInfo("websocket.session.accepted",
                     "ChatServer WebSocket session accepted",
                     {{"session_id", sessionId()}, {"path", _path}});
    doRead();
}

void WebSocketSession::doRead()
{
    if (_closed.load(std::memory_order_acquire))
    {
        return;
    }
    _buffer.consume(_buffer.size());
    auto self = Self();
    _ws.async_read(_buffer,
                   [self](beast::error_code ec, std::size_t bytes_transferred)
                   {
                       self->onRead(ec, bytes_transferred);
                   });
}

void WebSocketSession::onRead(beast::error_code ec, std::size_t)
{
    if (ec == websocket::error::closed)
    {
        finishClosed();
        return;
    }
    if (ec)
    {
        memolog::LogWarn("websocket.session.read_failed",
                         "WebSocket chat read failed",
                         {{"session_id", sessionId()}, {"error", ec.message()}});
        finishClosed();
        return;
    }

    if (_ws.got_text())
    {
        memolog::LogWarn("websocket.session.text_frame_rejected",
                         "WebSocket chat endpoint requires binary frames",
                         {{"session_id", sessionId()}});
        closeOnExecutor();
        return;
    }

    const auto bytes = beast::buffers_to_string(_buffer.data());
    if (bytes.size() > kMaxDecodeBufferBytes || _decode_buffer.size() > kMaxDecodeBufferBytes - bytes.size())
    {
        memolog::LogWarn("websocket.session.decode_buffer_too_large",
                         "WebSocket chat decode buffer exceeded limit",
                         {{"session_id", sessionId()},
                          {"buffer_size", std::to_string(_decode_buffer.size())},
                          {"incoming_size", std::to_string(bytes.size())}});
        closeOnExecutor();
        return;
    }
    _decode_buffer.insert(_decode_buffer.end(), bytes.begin(), bytes.end());
    _buffer.consume(_buffer.size());

    for (;;)
    {
        transport::ChatFrame frame;
        const auto status = transport::ChatFrameCodec::DecodeOne(_decode_buffer, frame);
        if (status == transport::ChatFrameDecodeStatus::NeedMoreData)
        {
            break;
        }
        if (status == transport::ChatFrameDecodeStatus::Invalid)
        {
            memolog::LogWarn("websocket.session.invalid_frame",
                             "WebSocket chat frame decode failed",
                             {{"session_id", sessionId()}});
            closeOnExecutor();
            return;
        }
        if (!_inbound_callback(Self(), frame.msg_id, frame.payload))
        {
            memolog::LogWarn("websocket.session.dispatch_rejected",
                             "WebSocket inbound chat frame dispatch failed",
                             {{"session_id", sessionId()}, {"msg_id", std::to_string(frame.msg_id)}});
            closeOnExecutor();
            return;
        }
    }

    doRead();
}

void WebSocketSession::enqueueSend(std::vector<std::uint8_t> frame)
{
    if (_closed.load(std::memory_order_acquire))
    {
        return;
    }
    const bool write_in_progress = !_send_queue.empty();
    _send_queue.push_back(std::move(frame));
    if (!write_in_progress)
    {
        doWrite();
    }
}

void WebSocketSession::doWrite()
{
    if (_closed.load(std::memory_order_acquire) || _send_queue.empty())
    {
        return;
    }
    _ws.binary(true);
    auto self = Self();
    _ws.async_write(net::buffer(_send_queue.front()),
                    [self](beast::error_code ec, std::size_t bytes_transferred)
                    {
                        self->onWrite(ec, bytes_transferred);
                    });
}

void WebSocketSession::onWrite(beast::error_code ec, std::size_t)
{
    if (ec)
    {
        memolog::LogWarn("websocket.session.write_failed",
                         "WebSocket chat write failed",
                         {{"session_id", sessionId()}, {"error", ec.message()}});
        finishClosed();
        return;
    }

    _send_queue.pop_front();
    if (!_send_queue.empty())
    {
        doWrite();
    }
}

void WebSocketSession::closeOnExecutor()
{
    if (_closed.exchange(true, std::memory_order_acq_rel))
    {
        return;
    }

    if (_ws.is_open())
    {
        auto self = Self();
        _ws.async_close(websocket::close_code::normal,
                        [self](beast::error_code)
                        {
                            self->finishClosed();
                        });
    }
    else
    {
        beast::error_code ignored;
        _ws.next_layer().shutdown(tcp::socket::shutdown_both, ignored);
        _ws.next_layer().close(ignored);
        finishClosed();
    }
}

void WebSocketSession::finishClosed()
{
    if (_close_notified.exchange(true, std::memory_order_acq_rel))
    {
        return;
    }
    _closed.store(true, std::memory_order_release);
    if (_close_callback)
    {
        _close_callback(sessionId());
    }
    if (userId() > 0)
    {
        CleanupExceptionSession(Self());
    }
}

} // namespace memochat::chatserver

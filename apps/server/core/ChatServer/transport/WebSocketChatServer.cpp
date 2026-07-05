#include "WebSocketChatServer.hpp"

#include "logging/Logger.hpp"

#include <cstdlib>
#include <utility>
#include <vector>

namespace memochat::chatserver
{

namespace net = boost::asio;
using tcp = net::ip::tcp;

WebSocketChatServer::WebSocketChatServer(boost::asio::io_context& io_context, InboundFrameCallback inbound_callback)
    : _io_context(io_context)
    , _acceptor(io_context)
    , _inbound_callback(std::move(inbound_callback))
{
}

WebSocketChatServer::~WebSocketChatServer()
{
    Stop();
}

bool WebSocketChatServer::Start(const std::string& host,
                                const std::string& port,
                                const std::string& path,
                                std::string* error)
{
    if (_running)
    {
        return true;
    }

    _host = host.empty() ? "127.0.0.1" : host;
    _port = port.empty() ? "8444" : port;
    _path = path.empty() ? "/ws" : path;

    boost::system::error_code ec;
    const auto address = net::ip::make_address(_host, ec);
    if (ec)
    {
        if (error)
        {
            *error = "websocket_invalid_host";
        }
        return false;
    }

    const int parsed_port = std::atoi(_port.c_str());
    if (parsed_port <= 0 || parsed_port > 65535)
    {
        if (error)
        {
            *error = "websocket_invalid_port";
        }
        return false;
    }

    const tcp::endpoint endpoint(address, static_cast<unsigned short>(parsed_port));
    _acceptor.open(endpoint.protocol(), ec);
    if (ec)
    {
        if (error)
        {
            *error = "websocket_acceptor_open_failed";
        }
        return false;
    }
    _acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec)
    {
        if (error)
        {
            *error = "websocket_reuse_address_failed";
        }
        return false;
    }
    _acceptor.bind(endpoint, ec);
    if (ec)
    {
        if (error)
        {
            *error = "websocket_bind_failed";
        }
        return false;
    }
    _acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec)
    {
        if (error)
        {
            *error = "websocket_listen_failed";
        }
        return false;
    }

    _running = true;
    memolog::LogInfo("websocket.ingress.start",
                     "ChatServer WebSocket ingress started",
                     {{"host", _host}, {"port", _port}, {"path", _path}});
    doAccept();
    return true;
}

void WebSocketChatServer::Stop()
{
    if (!_running && !_acceptor.is_open())
    {
        return;
    }
    _running = false;
    boost::system::error_code ignored;
    _acceptor.cancel(ignored);
    _acceptor.close(ignored);
    closeAllSessions();
    memolog::LogInfo("websocket.ingress.stop",
                     "ChatServer WebSocket ingress stopped",
                     {{"host", _host}, {"port", _port}, {"path", _path}});
}

bool WebSocketChatServer::isRunning() const
{
    return _running;
}

std::string WebSocketChatServer::listenHost() const
{
    return _host;
}

std::string WebSocketChatServer::listenPort() const
{
    return _port;
}

std::string WebSocketChatServer::listenPath() const
{
    return _path;
}

std::size_t WebSocketChatServer::sessionCount() const
{
    std::lock_guard<std::mutex> lock(_session_mutex);
    return _sessions.size();
}

void WebSocketChatServer::doAccept()
{
    auto self = shared_from_this();
    _acceptor.async_accept(
        [self](boost::system::error_code ec, tcp::socket socket)
        {
            if (!self->_running)
            {
                return;
            }
            if (ec)
            {
                memolog::LogWarn("websocket.ingress.accept_failed",
                                 "ChatServer WebSocket accept failed",
                                 {{"error", ec.message()}});
                self->doAccept();
                return;
            }

            auto session = std::make_shared<WebSocketSession>(
                std::move(socket),
                self->_path,
                [owner = std::weak_ptr<WebSocketChatServer>(self)](const std::string& session_id)
                {
                    if (auto locked = owner.lock())
                    {
                        locked->cleanupClosedSession(session_id);
                    }
                },
                self->_inbound_callback);

            {
                std::lock_guard<std::mutex> lock(self->_session_mutex);
                self->_sessions.emplace(session->sessionId(), session);
            }
            session->Start();
            self->doAccept();
        });
}

bool WebSocketChatServer::cleanupClosedSession(const std::string& session_id)
{
    std::lock_guard<std::mutex> lock(_session_mutex);
    return _sessions.erase(session_id) > 0;
}

void WebSocketChatServer::closeAllSessions()
{
    std::vector<std::shared_ptr<WebSocketSession>> sessions;
    {
        std::lock_guard<std::mutex> lock(_session_mutex);
        for (auto& [_, session] : _sessions)
        {
            sessions.push_back(session);
        }
        _sessions.clear();
    }
    for (const auto& session : sessions)
    {
        if (session)
        {
            session->Close();
        }
    }
}

} // namespace memochat::chatserver

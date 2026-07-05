#pragma once

#include "WebSocketSession.hpp"

#include <boost/asio.hpp>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace memochat::chatserver
{

class WebSocketChatServer : public std::enable_shared_from_this<WebSocketChatServer>
{
public:
    using InboundFrameCallback = WebSocketSession::InboundFrameCallback;

    explicit WebSocketChatServer(boost::asio::io_context& io_context, InboundFrameCallback inbound_callback = {});
    ~WebSocketChatServer();

    bool Start(const std::string& host, const std::string& port, const std::string& path, std::string* error = nullptr);
    void Stop();

    bool isRunning() const;
    std::string listenHost() const;
    std::string listenPort() const;
    std::string listenPath() const;
    std::size_t sessionCount() const;

private:
    void doAccept();
    bool cleanupClosedSession(const std::string& session_id);
    void closeAllSessions();

    boost::asio::io_context& _io_context;
    boost::asio::ip::tcp::acceptor _acceptor;
    bool _running = false;
    std::string _host;
    std::string _port;
    std::string _path = "/ws";
    InboundFrameCallback _inbound_callback;
    std::unordered_map<std::string, std::shared_ptr<WebSocketSession>> _sessions;
    mutable std::mutex _session_mutex;
};

} // namespace memochat::chatserver

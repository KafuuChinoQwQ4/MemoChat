#pragma once

#include <boost/asio/io_context.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

class QuicSession;

namespace memochat::chatserver {

class QuicChatServer {
public:
    explicit QuicChatServer(boost::asio::io_context& io_context);
    ~QuicChatServer();

    bool Start(const std::string& host, const std::string& port, std::string* error = nullptr);
    void Stop();

    bool isRunning() const;
    std::string listenHost() const;
    std::string listenPort() const;

private:
    struct Impl;

    bool cleanupClosedSession(const std::string& session_id);
    void closeAllSessions();

    boost::asio::io_context& _io_context;
    bool _running = false;
    std::string _host;
    std::string _port;
    std::unordered_map<std::string, std::shared_ptr<QuicSession>> _sessions;
    mutable std::mutex _session_mutex;
    std::unique_ptr<Impl> _impl;
};

} // namespace memochat::chatserver

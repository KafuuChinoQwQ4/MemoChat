#pragma once

#include "IWebTransportProvider.hpp"
#include "IWebTransportSession.hpp"

#include <boost/asio.hpp>

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

namespace memochat::chatserver
{

class WebTransportSession;

class WebTransportChatServer : public std::enable_shared_from_this<WebTransportChatServer>
{
public:
    explicit WebTransportChatServer(boost::asio::io_context& io_context);
    WebTransportChatServer(boost::asio::io_context& io_context, std::unique_ptr<IWebTransportProvider> provider);
    ~WebTransportChatServer();

    bool Start(const std::string& host, const std::string& port, const std::string& path, std::string* error = nullptr);
    bool Start(const WebTransportListenOptions& options, std::string* error = nullptr);
    void Stop();

    bool isRunning() const;
    std::string listenHost() const;
    std::string listenPort() const;
    std::string listenPath() const;
    std::string providerName() const;
    std::size_t sessionCount() const;

private:
    std::shared_ptr<IWebTransportSession> openProviderSession(WebTransportSendFrameCallback send_callback,
                                                              WebTransportCloseCallback close_callback);
    bool cleanupClosedSession(const std::string& session_id);
    void closeAllSessions();

    boost::asio::io_context& _io_context;
    std::unique_ptr<IWebTransportProvider> _provider;
    bool _running = false;
    std::string _host;
    std::string _port;
    std::string _path = "/chat";
    std::string _cert_file;
    std::string _private_key_file;
    std::unordered_map<std::string, std::shared_ptr<WebTransportSession>> _sessions;
    mutable std::mutex _session_mutex;
};

} // namespace memochat::chatserver

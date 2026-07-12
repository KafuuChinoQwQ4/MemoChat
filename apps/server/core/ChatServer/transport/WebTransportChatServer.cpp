#include "WebTransportChatServer.hpp"

#include "WebTransportProviderFactory.hpp"
#include "WebTransportSession.hpp"
#include "logging/Logger.hpp"

#include <utility>
#include <vector>

namespace memochat::chatserver
{

WebTransportChatServer::WebTransportChatServer(boost::asio::io_context& io_context)
    : WebTransportChatServer(io_context, nullptr)
{
}

WebTransportChatServer::WebTransportChatServer(boost::asio::io_context& io_context,
                                               std::unique_ptr<IWebTransportProvider> provider)
    : _io_context(io_context)
    , _provider(std::move(provider))
{
}

WebTransportChatServer::~WebTransportChatServer()
{
    Stop();
}

bool WebTransportChatServer::Start(const std::string& host,
                                   const std::string& port,
                                   const std::string& path,
                                   std::string* error)
{
    WebTransportListenOptions options;
    options.host = host;
    options.port = port;
    options.path = path;
    return Start(options, error);
}

bool WebTransportChatServer::Start(const WebTransportListenOptions& options, std::string* error)
{
    (void) _io_context;
    if (_running)
    {
        if (error)
        {
            error->clear();
        }
        return true;
    }
    if (error)
    {
        error->clear();
    }

    _host = options.host.empty() ? "127.0.0.1" : options.host;
    _port = options.port.empty() ? "8445" : options.port;
    _path = options.path.empty() ? "/chat" : options.path;
    _cert_file = options.cert_file;
    _private_key_file = options.private_key_file;

    if (!_provider)
    {
        _provider = CreateDefaultWebTransportProvider();
    }

    WebTransportProviderSessionHooks hooks;
    hooks.open_session = [owner = std::weak_ptr<WebTransportChatServer>(weak_from_this())](
                             WebTransportSendFrameCallback send_callback,
                             WebTransportCloseCallback close_callback) -> std::shared_ptr<IWebTransportSession>
    {
        if (auto locked = owner.lock())
        {
            return locked->openProviderSession(std::move(send_callback), std::move(close_callback));
        }
        return nullptr;
    };

    const WebTransportListenOptions listen_options{_host, _port, _path, _cert_file, _private_key_file};
    std::string provider_error;
    if (!_provider->Start(listen_options, std::move(hooks), &provider_error))
    {
        if (error)
        {
            *error = provider_error.empty() ? "webtransport_provider_start_failed" : provider_error;
        }
        memolog::LogWarn("webtransport.ingress.start_failed",
                         "ChatServer WebTransport provider start failed",
                         {{"provider", _provider->providerName()},
                          {"error", provider_error},
                          {"host", _host},
                          {"port", _port},
                          {"path", _path}});
        _running = false;
        return false;
    }

    _running = true;
    memolog::LogInfo("webtransport.ingress.start",
                     "ChatServer WebTransport ingress started",
                     {{"provider", _provider->providerName()}, {"host", _host}, {"port", _port}, {"path", _path}});
    return true;
}

void WebTransportChatServer::Stop()
{
    if (!_running && !_provider)
    {
        return;
    }
    _running = false;
    closeAllSessions();
    if (_provider)
    {
        _provider->Stop();
    }
}

bool WebTransportChatServer::isRunning() const
{
    return _running && _provider && _provider->isRunning();
}

std::string WebTransportChatServer::listenHost() const
{
    return _host;
}

std::string WebTransportChatServer::listenPort() const
{
    return _port;
}

std::string WebTransportChatServer::listenPath() const
{
    return _path;
}

std::string WebTransportChatServer::providerName() const
{
    return _provider ? _provider->providerName() : "unconfigured";
}

std::size_t WebTransportChatServer::sessionCount() const
{
    std::lock_guard<std::mutex> lock(_session_mutex);
    return _sessions.size();
}

std::shared_ptr<IWebTransportSession>
WebTransportChatServer::openProviderSession(WebTransportSendFrameCallback send_callback,
                                            WebTransportCloseCallback close_callback)
{
    auto session = std::make_shared<WebTransportSession>(
        std::move(send_callback),
        [owner = std::weak_ptr<WebTransportChatServer>(weak_from_this()),
         provider_close = std::move(close_callback)](const std::string& session_id)
        {
            if (provider_close)
            {
                provider_close(session_id);
            }
            if (auto locked = owner.lock())
            {
                locked->cleanupClosedSession(session_id);
            }
        });

    if (!session->Ready())
    {
        memolog::LogError("webtransport.session.uuid_failed",
                          "WebTransport session UUID generation failed",
                          {{"error", session->startupError()}});
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(_session_mutex);
        _sessions.emplace(session->sessionId(), session);
    }
    session->Start();
    memolog::LogInfo("webtransport.session.accepted",
                     "ChatServer WebTransport session accepted",
                     {{"session_id", session->sessionId()}, {"provider", providerName()}, {"path", _path}});
    return session;
}

bool WebTransportChatServer::cleanupClosedSession(const std::string& session_id)
{
    std::lock_guard<std::mutex> lock(_session_mutex);
    return _sessions.erase(session_id) > 0;
}

void WebTransportChatServer::closeAllSessions()
{
    std::vector<std::shared_ptr<WebTransportSession>> sessions;
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

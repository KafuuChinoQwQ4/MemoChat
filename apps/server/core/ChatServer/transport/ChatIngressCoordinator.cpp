#include "ChatIngressCoordinator.hpp"

#include "CServer.hpp"
#include "ConfigMgr.hpp"
#include "QuicChatServer.hpp"
#include "WebSocketChatServer.hpp"
#include "WebTransportChatServer.hpp"
#include "IWebTransportProvider.hpp"
#include "logging/Logger.hpp"

#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <charconv>

namespace memochat::chatserver
{

namespace
{
std::string LowerAscii(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool IsTruthyValue(const std::string& value)
{
    const auto lower = LowerAscii(value);
    return lower == "1" || lower == "true" || lower == "yes" || lower == "on";
}

bool IsFalsyValue(const std::string& value)
{
    const auto lower = LowerAscii(value);
    return lower == "0" || lower == "false" || lower == "no" || lower == "off";
}

bool IsTruthyFlag(const char* raw)
{
    if (raw == nullptr)
    {
        return true;
    }
    return IsTruthyValue(raw);
}

bool FeatureEnabled(const std::string& section, const std::string& key, const char* env_name, bool default_value)
{
    if (env_name != nullptr)
    {
        if (const char* env_value = std::getenv(env_name))
        {
            const std::string value(env_value);
            if (IsTruthyValue(value))
            {
                return true;
            }
            if (IsFalsyValue(value))
            {
                return false;
            }
        }
    }

    const auto value = ConfigMgr::Inst().GetValue(section, key);
    if (value.empty())
    {
        return default_value;
    }
    if (IsTruthyValue(value))
    {
        return true;
    }
    if (IsFalsyValue(value))
    {
        return false;
    }
    return default_value;
}

std::string ConfigValueOr(const std::string& section, const std::string& key, const std::string& fallback)
{
    auto value = ConfigMgr::Inst().GetValue(section, key);
    return value.empty() ? fallback : value;
}
} // namespace

ChatIngressCoordinator::ChatIngressCoordinator(boost::asio::io_context& io_context)
    : _io_context(io_context)
{
}

ChatIngressCoordinator::~ChatIngressCoordinator()
{
    Stop();
}

bool ChatIngressCoordinator::Start(const memochat::cluster::ChatNodeDescriptor& self_node, std::string* error)
{
    unsigned int tcp_port = 0;
    const auto parsed =
        std::from_chars(self_node.tcp_port.data(), self_node.tcp_port.data() + self_node.tcp_port.size(), tcp_port);
    if (parsed.ec != std::errc{} || parsed.ptr != self_node.tcp_port.data() + self_node.tcp_port.size() ||
        tcp_port == 0 || tcp_port > 65535)
    {
        if (error != nullptr)
        {
            *error = "invalid TCP ingress port: " + self_node.tcp_port;
        }
        return false;
    }
    _tcp_server = std::make_shared<CServer>(_io_context, static_cast<unsigned short>(tcp_port));
    if (!_tcp_server->Ready())
    {
        if (error != nullptr)
        {
            *error = _tcp_server->startupError();
        }
        _tcp_server.reset();
        return false;
    }
    _tcp_server->Start();
    _tcp_server->StartTimer();

    if (FeatureEnabled(self_node.name, "WsEnabled", "MEMOCHAT_ENABLE_WS", false))
    {
        const auto ws_host = ConfigValueOr(self_node.name, "WsHost", self_node.tcp_host);
        const auto ws_port = ConfigValueOr(self_node.name, "WsPort", "8444");
        const auto ws_path = ConfigValueOr(self_node.name, "WsPath", "/ws");
        _websocket_server = std::make_shared<WebSocketChatServer>(_io_context);
        std::string ws_error;
        if (!_websocket_server->Start(ws_host, ws_port, ws_path, &ws_error))
        {
            memolog::LogWarn("websocket.ingress.start_failed",
                             "ChatServer WebSocket ingress start failed",
                             {{"name", self_node.name}, {"error", ws_error}, {"host", ws_host}, {"port", ws_port}});
            _websocket_server.reset();
        }
    }
    else
    {
        memolog::LogInfo("websocket.ingress.disabled",
                         "ChatServer WebSocket ingress disabled",
                         {{"name", self_node.name}});
    }

    if (FeatureEnabled(self_node.name, "WtEnabled", "MEMOCHAT_ENABLE_WEBTRANSPORT", false))
    {
        const auto wt_host = ConfigValueOr(self_node.name, "WtHost", self_node.tcp_host);
        const auto wt_port = ConfigValueOr(self_node.name, "WtPort", "8445");
        const auto wt_path = ConfigValueOr(self_node.name, "WtPath", "/chat");
        const auto wt_cert_file = ConfigValueOr(self_node.name, "WtCertFile", "");
        const auto wt_key_file = ConfigValueOr(self_node.name, "WtKeyFile", "");
        WebTransportListenOptions wt_options;
        wt_options.host = wt_host;
        wt_options.port = wt_port;
        wt_options.path = wt_path;
        wt_options.cert_file = wt_cert_file;
        wt_options.private_key_file = wt_key_file;
        _webtransport_server = std::make_shared<WebTransportChatServer>(_io_context);
        std::string wt_error;
        if (!_webtransport_server->Start(wt_options, &wt_error))
        {
            memolog::LogWarn("webtransport.ingress.start_failed",
                             "ChatServer WebTransport ingress start failed",
                             {{"name", self_node.name}, {"error", wt_error}, {"host", wt_host}, {"port", wt_port}});
            _webtransport_server.reset();
        }
    }
    else
    {
        memolog::LogInfo("webtransport.ingress.disabled",
                         "ChatServer WebTransport ingress disabled",
                         {{"name", self_node.name}});
    }

    const bool quic_enabled = IsTruthyFlag(std::getenv("MEMOCHAT_ENABLE_QUIC"));
    if (!quic_enabled)
    {
        memolog::LogInfo("quic.ingress.disabled",
                         "ChatServer QUIC ingress disabled by env",
                         {{"name", self_node.name}});
        return true;
    }

    if (self_node.quic_host.empty() || self_node.quic_port.empty())
    {
        memolog::LogInfo("quic.ingress.missing_endpoint",
                         "ChatServer QUIC endpoint not configured",
                         {{"name", self_node.name}});
        return true;
    }

    _quic_server = std::make_shared<QuicChatServer>(_io_context);
    std::string quic_error;
    if (!_quic_server->Start(self_node.quic_host, self_node.quic_port, &quic_error))
    {
        memolog::LogWarn("quic.ingress.start_failed",
                         "ChatServer QUIC ingress start failed",
                         {{"name", self_node.name},
                          {"error", quic_error},
                          {"host", self_node.quic_host},
                          {"port", self_node.quic_port}});
        _quic_server.reset();
    }
    return true;
}

void ChatIngressCoordinator::Stop()
{
    if (_webtransport_server)
    {
        _webtransport_server->Stop();
        _webtransport_server.reset();
    }
    if (_websocket_server)
    {
        _websocket_server->Stop();
        _websocket_server.reset();
    }
    if (_quic_server)
    {
        _quic_server->Stop();
        _quic_server.reset();
    }
    if (_tcp_server)
    {
        _tcp_server->StopTimer();
        _tcp_server.reset();
    }
}

std::shared_ptr<CServer> ChatIngressCoordinator::tcpServer() const
{
    return _tcp_server;
}

std::shared_ptr<QuicChatServer> ChatIngressCoordinator::quicServer() const
{
    return _quic_server;
}

std::shared_ptr<WebSocketChatServer> ChatIngressCoordinator::webSocketServer() const
{
    return _websocket_server;
}

std::shared_ptr<WebTransportChatServer> ChatIngressCoordinator::webTransportServer() const
{
    return _webtransport_server;
}

} // namespace memochat::chatserver

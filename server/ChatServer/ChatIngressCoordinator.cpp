#include "ChatIngressCoordinator.h"

#include "CServer.h"
#include "QuicChatServer.h"
#include "logging/Logger.h"

#include <cstdlib>

namespace memochat::chatserver {

namespace {
bool IsTruthyFlag(const char* raw)
{
    if (raw == nullptr) {
        return true;
    }
    const std::string value(raw);
    return value == "1" || value == "true" || value == "TRUE" || value == "on" || value == "ON";
}
}

ChatIngressCoordinator::ChatIngressCoordinator(boost::asio::io_context& io_context)
    : _io_context(io_context)
{
}

ChatIngressCoordinator::~ChatIngressCoordinator()
{
    Stop();
}

void ChatIngressCoordinator::Start(const memochat::cluster::ChatNodeDescriptor& self_node)
{
    _tcp_server = std::make_shared<CServer>(_io_context, static_cast<short>(std::atoi(self_node.tcp_port.c_str())));
    _tcp_server->StartTimer();

    const bool quic_enabled = IsTruthyFlag(std::getenv("MEMOCHAT_ENABLE_QUIC"));
    if (!quic_enabled) {
        memolog::LogInfo("quic.ingress.disabled", "ChatServer QUIC ingress disabled by env",
            {{"name", self_node.name}});
        return;
    }

    if (self_node.quic_host.empty() || self_node.quic_port.empty()) {
        memolog::LogInfo("quic.ingress.missing_endpoint", "ChatServer QUIC endpoint not configured",
            {{"name", self_node.name}});
        return;
    }

    _quic_server = std::make_shared<QuicChatServer>(_io_context);
    std::string error;
    if (!_quic_server->Start(self_node.quic_host, self_node.quic_port, &error)) {
        memolog::LogWarn("quic.ingress.start_failed", "ChatServer QUIC ingress start failed",
            {{"name", self_node.name}, {"error", error}, {"host", self_node.quic_host}, {"port", self_node.quic_port}});
        _quic_server.reset();
    }
}

void ChatIngressCoordinator::Stop()
{
    if (_quic_server) {
        _quic_server->Stop();
        _quic_server.reset();
    }
    if (_tcp_server) {
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

} // namespace memochat::chatserver

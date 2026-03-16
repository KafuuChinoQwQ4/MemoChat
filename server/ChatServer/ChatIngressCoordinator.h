#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>

#include "cluster/ChatClusterDiscovery.h"

class CServer;

namespace memochat::chatserver {

class QuicChatServer;

class ChatIngressCoordinator {
public:
    explicit ChatIngressCoordinator(boost::asio::io_context& io_context);
    ~ChatIngressCoordinator();

    void Start(const memochat::cluster::ChatNodeDescriptor& self_node);
    void Stop();

    std::shared_ptr<CServer> tcpServer() const;
    std::shared_ptr<QuicChatServer> quicServer() const;

private:
    boost::asio::io_context& _io_context;
    std::shared_ptr<CServer> _tcp_server;
    std::shared_ptr<QuicChatServer> _quic_server;
};

} // namespace memochat::chatserver

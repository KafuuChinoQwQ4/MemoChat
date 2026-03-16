#include "TransportSelector.h"

#include <QDebug>

#include "QuicChatTransport.h"
#include "tcpmgr.h"

namespace {
QString transportKindName(ChatTransportKind kind)
{
    return kind == ChatTransportKind::Quic ? QStringLiteral("quic") : QStringLiteral("tcp");
}
}

TransportSelector::TransportSelector(QObject *parent)
    : IChatTransport(parent),
      _active_transport(TcpMgr::GetInstance()),
      _dispatcher(std::make_shared<ChatMessageDispatcher>(this))
{
    bindTransport(TcpMgr::GetInstance());
    bindTransport(QuicChatTransport::GetInstance());
}

void TransportSelector::bindTransport(const std::shared_ptr<IChatTransport> &transport)
{
    if (!transport) {
        return;
    }

    QObject::connect(transport.get(), &IChatTransport::sig_con_success,
                     this,
                     [this, transport](bool success) {
                         handleTransportConnectResult(transport, success);
                     });
    QObject::connect(transport.get(), &IChatTransport::sig_connection_closed,
                     this,
                     [this, transport]() {
                         handleTransportClosed(transport);
                     });
    QObject::connect(transport.get(), &IChatTransport::sig_message_received,
                     this,
                     [this, transport](ReqId reqId, int len, const QByteArray &data) {
                         if (transport != _active_transport) {
                             return;
                         }
                         _dispatcher->dispatchMessage(reqId, len, data);
                     });
}

void TransportSelector::handleTransportConnectResult(const std::shared_ptr<IChatTransport> &transport, bool success)
{
    if (transport != _active_transport) {
        return;
    }

    if (success) {
        _connecting = false;
        emit sig_con_success(true);
        return;
    }

    if (_connecting && !_fallback_attempted && _active_kind != _pending_server_info.FallbackTransport) {
        _fallback_attempted = true;
        if (tryActivateTransport(_pending_server_info.FallbackTransport)) {
            return;
        }
    }

    _connecting = false;
    emit sig_con_success(false);
}

void TransportSelector::handleTransportClosed(const std::shared_ptr<IChatTransport> &transport)
{
    if (transport != _active_transport) {
        return;
    }
    _connecting = false;
    emit sig_connection_closed();
}

ChatEndpoint TransportSelector::resolveEndpoint(ChatTransportKind kind) const
{
    for (const auto &endpoint : _pending_server_info.Endpoints) {
        if (endpoint.transport == kind) {
            return endpoint;
        }
    }

    ChatEndpoint fallback;
    fallback.transport = kind;
    fallback.host = _pending_server_info.Host;
    fallback.port = _pending_server_info.Port;
    fallback.serverName = _pending_server_info.ServerName;
    return fallback;
}

bool TransportSelector::tryActivateTransport(ChatTransportKind kind)
{
    const ChatEndpoint endpoint = resolveEndpoint(kind);
    if (endpoint.host.trimmed().isEmpty() || endpoint.port.trimmed().isEmpty()) {
        return false;
    }

    auto transport = transportForKind(kind);
    if (!transport) {
        return false;
    }

    _active_transport = transport;
    _active_kind = kind;

    ServerInfo connectInfo = _pending_server_info;
    connectInfo.Host = endpoint.host;
    connectInfo.Port = endpoint.port;
    connectInfo.ServerName = endpoint.serverName;
    qInfo() << "transport selector activating transport:" << transportKindName(kind)
            << "host:" << connectInfo.Host
            << "port:" << connectInfo.Port;
    transport->connectToServer(connectInfo);
    return true;
}

std::shared_ptr<IChatTransport> TransportSelector::transportForKind(ChatTransportKind kind) const
{
    if (kind == ChatTransportKind::Quic) {
        return QuicChatTransport::GetInstance();
    }
    return TcpMgr::GetInstance();
}

void TransportSelector::CloseConnection()
{
    _connecting = false;
    if (_active_transport) {
        _active_transport->CloseConnection();
    }
}

bool TransportSelector::isConnected() const
{
    return _active_transport && _active_transport->isConnected();
}

void TransportSelector::connectToServer(ServerInfo serverInfo)
{
    _pending_server_info = serverInfo;
    _fallback_attempted = false;
    _connecting = true;

    if (!tryActivateTransport(serverInfo.PreferredTransport)) {
        _fallback_attempted = true;
        if (!tryActivateTransport(serverInfo.FallbackTransport)) {
            _connecting = false;
            emit sig_con_success(false);
        }
    }
}

void TransportSelector::slot_send_data(ReqId reqId, QByteArray data)
{
    if (_active_transport) {
        _active_transport->slot_send_data(reqId, data);
    }
}

std::shared_ptr<ChatMessageDispatcher> TransportSelector::dispatcher() const
{
    return _dispatcher;
}

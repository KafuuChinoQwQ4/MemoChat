#include "TransportSelector.h"

#include <QDebug>

#include "QuicChatTransport.h"
#include "tcpmgr.h"

namespace {
QString transportKindName(ChatTransportKind kind)
{
    return kind == ChatTransportKind::Quic ? QStringLiteral("quic") : QStringLiteral("tcp");
}

bool sameEndpoint(const ChatEndpoint &a, const ChatEndpoint &b)
{
    return a.transport == b.transport &&
           a.host.compare(b.host, Qt::CaseInsensitive) == 0 &&
           a.port == b.port;
}

void appendUniqueEndpoint(QVector<ChatEndpoint> &endpoints, const ChatEndpoint &endpoint)
{
    if (endpoint.host.trimmed().isEmpty() || endpoint.port.trimmed().isEmpty()) {
        return;
    }
    for (const auto &existing : endpoints) {
        if (sameEndpoint(existing, endpoint)) {
            return;
        }
    }
    endpoints.push_back(endpoint);
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

    if (_connecting && tryNextEndpoint()) {
        return;
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
    return tryActivateEndpoint(endpoint);
}

bool TransportSelector::tryActivateEndpoint(const ChatEndpoint &endpoint)
{
    if (endpoint.host.trimmed().isEmpty() || endpoint.port.trimmed().isEmpty()) {
        return false;
    }

    auto transport = transportForKind(endpoint.transport);
    if (!transport) {
        return false;
    }

    _active_transport = transport;
    _active_kind = endpoint.transport;

    ServerInfo connectInfo = _pending_server_info;
    connectInfo.Host = endpoint.host;
    connectInfo.Port = endpoint.port;
    connectInfo.ServerName = endpoint.serverName;
    connectInfo.PreferredTransport = endpoint.transport;
    connectInfo.FallbackTransport = _pending_server_info.FallbackTransport;
    qInfo() << "transport selector activating transport:" << transportKindName(endpoint.transport)
            << "host:" << connectInfo.Host
            << "port:" << connectInfo.Port
            << "candidate:" << (_candidate_index + 1)
            << "/" << _candidate_endpoints.size();
    transport->connectToServer(connectInfo);
    return true;
}

bool TransportSelector::tryNextEndpoint()
{
    while (_candidate_index + 1 < _candidate_endpoints.size()) {
        ++_candidate_index;
        if (tryActivateEndpoint(_candidate_endpoints.at(_candidate_index))) {
            return true;
        }
    }
    return false;
}

QVector<ChatEndpoint> TransportSelector::buildCandidateEndpoints(const ServerInfo &serverInfo) const
{
    QVector<ChatEndpoint> candidates;
    const auto appendByKind = [&candidates, &serverInfo](ChatTransportKind kind) {
        for (const auto &endpoint : serverInfo.Endpoints) {
            if (endpoint.transport == kind) {
                appendUniqueEndpoint(candidates, endpoint);
            }
        }
    };

    appendByKind(serverInfo.PreferredTransport);
    if (serverInfo.FallbackTransport != serverInfo.PreferredTransport) {
        appendByKind(serverInfo.FallbackTransport);
    }
    for (const auto &endpoint : serverInfo.Endpoints) {
        appendUniqueEndpoint(candidates, endpoint);
    }

    if (candidates.isEmpty() && !serverInfo.Host.trimmed().isEmpty() && !serverInfo.Port.trimmed().isEmpty()) {
        ChatEndpoint fallback;
        fallback.transport = serverInfo.PreferredTransport;
        fallback.host = serverInfo.Host;
        fallback.port = serverInfo.Port;
        fallback.serverName = serverInfo.ServerName;
        appendUniqueEndpoint(candidates, fallback);
    }
    return candidates;
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
    _candidate_endpoints = buildCandidateEndpoints(serverInfo);
    _candidate_index = 0;
    _connecting = true;

    if (_candidate_endpoints.isEmpty() || !tryActivateEndpoint(_candidate_endpoints.front())) {
        if (!tryNextEndpoint()) {
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

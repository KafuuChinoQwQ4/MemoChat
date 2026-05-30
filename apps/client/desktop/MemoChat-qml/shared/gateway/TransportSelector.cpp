#include "TransportSelector.h"

#include <QDebug>

#include "QuicChatTransport.h"
#include "TransportEndpointPolicy.h"
#include "tcpmgr.h"

TransportSelector::TransportSelector(QObject* parent)
    : IChatTransport(parent)
    , _active_transport(TcpMgr::GetInstance())
    , _dispatcher(std::make_shared<ChatMessageDispatcher>(this))
{
    bindTransport(TcpMgr::GetInstance());
    bindTransport(QuicChatTransport::GetInstance());
}

void TransportSelector::bindTransport(const std::shared_ptr<IChatTransport>& transport)
{
    if (!transport)
    {
        return;
    }

    QObject::connect(transport.get(),
                     &IChatTransport::sig_con_success,
                     this,
                     [this, transport](bool success)
                     {
                         handleTransportConnectResult(transport, success);
                     });
    QObject::connect(transport.get(),
                     &IChatTransport::sig_connection_closed,
                     this,
                     [this, transport]()
                     {
                         handleTransportClosed(transport);
                     });
    QObject::connect(transport.get(),
                     &IChatTransport::sig_message_received,
                     this,
                     [this, transport](ReqId reqId, int len, const QByteArray& data)
                     {
                         if (transport != _active_transport)
                         {
                             return;
                         }
                         _dispatcher->dispatchMessage(reqId, len, data);
                     });
}

void TransportSelector::handleTransportConnectResult(const std::shared_ptr<IChatTransport>& transport, bool success)
{
    if (transport != _active_transport)
    {
        return;
    }

    if (success)
    {
        _connecting = false;
        emit sig_con_success(true);
        return;
    }

    if (_connecting && tryNextEndpoint())
    {
        return;
    }

    _connecting = false;
    emit sig_con_success(false);
}

void TransportSelector::handleTransportClosed(const std::shared_ptr<IChatTransport>& transport)
{
    if (transport != _active_transport)
    {
        return;
    }
    _connecting = false;
    emit sig_connection_closed();
}

bool TransportSelector::tryActivateEndpoint(const ChatEndpoint& endpoint)
{
    if (endpoint.host.trimmed().isEmpty() || endpoint.port.trimmed().isEmpty())
    {
        return false;
    }

    auto transport = transportForKind(endpoint.transport);
    if (!transport)
    {
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
            << "host:" << connectInfo.Host << "port:" << connectInfo.Port << "candidate:" << (_candidate_index + 1)
            << "/" << _candidate_endpoints.size();
    transport->connectToServer(connectInfo);
    return true;
}

bool TransportSelector::tryNextEndpoint()
{
    while (_candidate_index + 1 < _candidate_endpoints.size())
    {
        ++_candidate_index;
        if (tryActivateEndpoint(_candidate_endpoints.at(_candidate_index)))
        {
            return true;
        }
    }
    return false;
}

std::shared_ptr<IChatTransport> TransportSelector::transportForKind(ChatTransportKind kind) const
{
    if (kind == ChatTransportKind::Quic)
    {
        return QuicChatTransport::GetInstance();
    }
    return TcpMgr::GetInstance();
}

void TransportSelector::CloseConnection()
{
    _connecting = false;
    if (_active_transport)
    {
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
    _candidate_endpoints = buildCandidateChatEndpoints(serverInfo);
    _candidate_index = 0;
    _connecting = true;

    if (_candidate_endpoints.isEmpty() || !tryActivateEndpoint(_candidate_endpoints.front()))
    {
        if (!tryNextEndpoint())
        {
            _connecting = false;
            emit sig_con_success(false);
        }
    }
}

void TransportSelector::slot_send_data(ReqId reqId, QByteArray data)
{
    if (_active_transport)
    {
        _active_transport->slot_send_data(reqId, data);
    }
}

std::shared_ptr<ChatMessageDispatcher> TransportSelector::dispatcher() const
{
    return _dispatcher;
}

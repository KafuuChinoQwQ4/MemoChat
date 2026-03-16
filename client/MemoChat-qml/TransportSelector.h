#ifndef TRANSPORTSELECTOR_H
#define TRANSPORTSELECTOR_H

#include <QObject>
#include <memory>

#include "ChatMessageDispatcher.h"
#include "IChatTransport.h"

class TransportSelector : public IChatTransport
{
    Q_OBJECT
public:
    explicit TransportSelector(QObject *parent = nullptr);
    ~TransportSelector() override = default;

    void CloseConnection() override;
    bool isConnected() const override;
    void connectToServer(ServerInfo serverInfo) override;
    void slot_send_data(ReqId reqId, QByteArray data) override;

    std::shared_ptr<ChatMessageDispatcher> dispatcher() const;

private:
    void bindTransport(const std::shared_ptr<IChatTransport> &transport);
    void handleTransportConnectResult(const std::shared_ptr<IChatTransport> &transport, bool success);
    void handleTransportClosed(const std::shared_ptr<IChatTransport> &transport);
    ChatEndpoint resolveEndpoint(ChatTransportKind kind) const;
    bool tryActivateTransport(ChatTransportKind kind);
    std::shared_ptr<IChatTransport> transportForKind(ChatTransportKind kind) const;

    ServerInfo _pending_server_info;
    std::shared_ptr<IChatTransport> _active_transport;
    std::shared_ptr<ChatMessageDispatcher> _dispatcher;
    bool _connecting = false;
    bool _fallback_attempted = false;
    ChatTransportKind _active_kind = ChatTransportKind::Tcp;
};

#endif // TRANSPORTSELECTOR_H

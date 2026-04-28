#ifndef QUICCHATTRANSPORT_H
#define QUICCHATTRANSPORT_H

#include <QByteArray>
#include <QTimer>

#include "IChatTransport.h"
#include "singleton.h"

#if MEMOCHAT_HAS_MSQUIC
#include <msquic.h>
#endif

class QuicChatTransport : public IChatTransport, public Singleton<QuicChatTransport>
{
    Q_OBJECT
public:
    ~QuicChatTransport() override;
    void CloseConnection() override;
    bool isConnected() const override;
    void connectToServer(ServerInfo serverInfo) override;
    void slot_send_data(ReqId reqId, QByteArray data) override;

private:
    friend class Singleton<QuicChatTransport>;
    QuicChatTransport();

    void resetParser();
    void handleReceivedBytes(const QByteArray &bytes);

#if MEMOCHAT_HAS_MSQUIC
    struct SendContext;
    bool ensureQuicReady(QString *errorText = nullptr);
    void closeHandles();
    static int QUIC_API connectionCallback(HQUIC connection, void *context, void *event);
    static int QUIC_API streamCallback(HQUIC stream, void *context, void *event);
    int onConnectionEvent(void *event);
    int onStreamEvent(void *event);

    const QUIC_API_TABLE *_api = nullptr;
    HQUIC _registration = nullptr;
    HQUIC _configuration = nullptr;
    HQUIC _connection = nullptr;
    HQUIC _stream = nullptr;
    QByteArray _alpn;
#endif

    QTimer _connect_timer;
    bool _connecting = false;
    bool _connected = false;
    bool _streamReady = false;
    bool _recvPending = false;
    quint16 _messageId = 0;
    quint16 _messageLen = 0;
    QByteArray _recvBuffer;
    QString _host;
    QString _serverName;
    uint16_t _port = 0;
};

#endif // QUICCHATTRANSPORT_H

#ifndef TCPMGR_H
#define TCPMGR_H
#include <QTimer>
#include <QTcpSocket>

#include "ChatFrameCodec.h"
#include "IChatTransport.h"
#include "singleton.h"

class TcpMgr
    : public IChatTransport
    , public Singleton<TcpMgr>
    , public std::enable_shared_from_this<TcpMgr>
{
    Q_OBJECT
public:
    ~TcpMgr() override;
    void CloseConnection() override;
    bool isConnected() const override;
    void connectToServer(ServerInfo serverInfo) override;
    void slot_send_data(ReqId reqId, QByteArray data) override;

private:
    friend class Singleton<TcpMgr>;
    TcpMgr();
    void handleMsg(ReqId id, int len, QByteArray data);
    QTcpSocket _socket;
    QString _host;
    uint16_t _port;
    ChatFrameParser _frame_parser;
    bool _connecting;
    QTimer _connect_timeout_timer;
signals:
    void sig_send_data(ReqId reqId, QByteArray data);
};

#endif // TCPMGR_H

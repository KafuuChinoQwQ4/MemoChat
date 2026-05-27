#ifndef ICHATTRANSPORT_H
#define ICHATTRANSPORT_H

#include <QObject>
#include <QByteArray>

#include "global.h"

class IChatTransport : public QObject
{
    Q_OBJECT
public:
    explicit IChatTransport(QObject *parent = nullptr) : QObject(parent) {}
    ~IChatTransport() override = default;

    virtual void CloseConnection() = 0;
    virtual bool isConnected() const = 0;
    virtual void connectToServer(ServerInfo serverInfo) = 0;
    virtual void slot_send_data(ReqId reqId, QByteArray data) = 0;

signals:
    void sig_con_success(bool bsuccess);
    void sig_message_received(ReqId reqId, int len, QByteArray data);
    void sig_connection_closed();
};

#endif // ICHATTRANSPORT_H

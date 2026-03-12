#ifndef CALLCONTROLLER_H
#define CALLCONTROLLER_H

#include <QString>
#include "global.h"

class ClientGateway;

class CallController
{
public:
    explicit CallController(ClientGateway *gateway);

    void startCall(int uid, const QString &token, int peerUid, const QString &callType) const;
    void acceptCall(int uid, const QString &token, const QString &callId) const;
    void rejectCall(int uid, const QString &token, const QString &callId) const;
    void cancelCall(int uid, const QString &token, const QString &callId) const;
    void hangupCall(int uid, const QString &token, const QString &callId) const;
    void fetchToken(int uid, const QString &token, const QString &callId, const QString &role) const;

private:
    void post(const QString &path, const QJsonObject &payload, ReqId reqId) const;

    ClientGateway *_gateway = nullptr;
};

#endif // CALLCONTROLLER_H

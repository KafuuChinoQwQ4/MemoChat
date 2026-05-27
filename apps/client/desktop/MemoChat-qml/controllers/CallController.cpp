#include "CallController.h"

#include "ClientGateway.h"
#include "httpmgr.h"
#include "global.h"
#include <QJsonObject>

CallController::CallController(ClientGateway *gateway)
    : _gateway(gateway)
{
}

void CallController::startCall(int uid, const QString &token, int peerUid, const QString &callType) const
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["token"] = token;
    payload["peer_uid"] = peerUid;
    payload["call_type"] = callType;
    post(QStringLiteral("/api/call/start"), payload, ReqId::ID_CALL_START);
}

void CallController::acceptCall(int uid, const QString &token, const QString &callId) const
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["token"] = token;
    payload["call_id"] = callId;
    post(QStringLiteral("/api/call/accept"), payload, ReqId::ID_CALL_ACCEPT);
}

void CallController::rejectCall(int uid, const QString &token, const QString &callId) const
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["token"] = token;
    payload["call_id"] = callId;
    post(QStringLiteral("/api/call/reject"), payload, ReqId::ID_CALL_REJECT);
}

void CallController::cancelCall(int uid, const QString &token, const QString &callId) const
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["token"] = token;
    payload["call_id"] = callId;
    post(QStringLiteral("/api/call/cancel"), payload, ReqId::ID_CALL_CANCEL);
}

void CallController::hangupCall(int uid, const QString &token, const QString &callId) const
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["token"] = token;
    payload["call_id"] = callId;
    post(QStringLiteral("/api/call/hangup"), payload, ReqId::ID_CALL_HANGUP);
}

void CallController::fetchToken(int uid, const QString &token, const QString &callId, const QString &role) const
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["token"] = token;
    payload["call_id"] = callId;
    payload["role"] = role;
    post(QStringLiteral("/api/call/token"), payload, ReqId::ID_CALL_GET_TOKEN);
}

void CallController::post(const QString &path, const QJsonObject &payload, ReqId reqId) const
{
    if (!_gateway || !_gateway->httpMgr()) {
        return;
    }
    _gateway->httpMgr()->PostHttpReq(QUrl(gate_url_prefix + path), payload, reqId, Modules::CALLMOD, QStringLiteral("call"));
}

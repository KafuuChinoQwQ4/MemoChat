#include "CallRequestPayloads.h"

namespace memochat::call_request_payload
{
QJsonObject buildStartCallPayload(int uid, const QString& token, int peerUid, const QString& callType)
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["token"] = token;
    payload["peer_uid"] = peerUid;
    payload["call_type"] = callType;
    return payload;
}

QJsonObject buildCallIdPayload(int uid, const QString& token, const QString& callId)
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["token"] = token;
    payload["call_id"] = callId;
    return payload;
}

QJsonObject buildFetchTokenPayload(int uid, const QString& token, const QString& callId, const QString& role)
{
    QJsonObject payload = buildCallIdPayload(uid, token, callId);
    payload["role"] = role;
    return payload;
}
} // namespace memochat::call_request_payload

#include "CallRequestPayloads.h"

#include <QtGlobal>

namespace memochat::call_request_payload
{
QJsonObject buildStartCallPayload(int uid, const QString& token, int peerUid, const QString& callType)
{
    Q_UNUSED(uid);
    Q_UNUSED(token);
    QJsonObject payload;
    payload["peer_uid"] = peerUid;
    payload["call_type"] = callType;
    return payload;
}

QJsonObject buildCallIdPayload(int uid, const QString& token, const QString& callId)
{
    Q_UNUSED(uid);
    Q_UNUSED(token);
    QJsonObject payload;
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

#include "CallRequestPayloads.h"

namespace memochat::call_request_payload
{
QJsonObject buildStartCallPayload(int peerUid, const QString& callType)
{
    QJsonObject payload;
    payload["peer_uid"] = peerUid;
    payload["call_type"] = callType;
    return payload;
}

QJsonObject buildCallIdPayload(const QString& callId)
{
    QJsonObject payload;
    payload["call_id"] = callId;
    return payload;
}

QJsonObject buildFetchTokenPayload(const QString& callId, const QString& role)
{
    QJsonObject payload = buildCallIdPayload(callId);
    payload["role"] = role;
    return payload;
}
} // namespace memochat::call_request_payload

#ifndef CALLREQUESTPAYLOADS_H
#define CALLREQUESTPAYLOADS_H

#include <QJsonObject>
#include <QString>

namespace memochat::call_request_payload
{
QJsonObject buildStartCallPayload(int peerUid, const QString& callType);
QJsonObject buildCallIdPayload(const QString& callId);
QJsonObject buildFetchTokenPayload(const QString& callId, const QString& role);
} // namespace memochat::call_request_payload

#endif // CALLREQUESTPAYLOADS_H

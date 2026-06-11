#ifndef CALLREQUESTPAYLOADS_H
#define CALLREQUESTPAYLOADS_H

#include <QJsonObject>
#include <QString>

namespace memochat::call_request_payload
{
QJsonObject buildStartCallPayload(int uid, const QString& token, int peerUid, const QString& callType);
QJsonObject buildCallIdPayload(int uid, const QString& token, const QString& callId);
QJsonObject buildFetchTokenPayload(int uid, const QString& token, const QString& callId, const QString& role);
} // namespace memochat::call_request_payload

#endif // CALLREQUESTPAYLOADS_H

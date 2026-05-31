#pragma once

#include <QJsonObject>
#include <QString>

namespace memochat::call_payload
{
struct CallEventPeer
{
    QString name;
    QString icon;
    bool isCaller = false;
};

QString mapCallError(int errorCode);
QString callStateText(const QString& eventType);
QJsonObject buildLivekitMetadata(const QString& callId,
                                 const QString& roomName,
                                 const QString& callType,
                                 const QString& peerName,
                                 const QString& peerIcon,
                                 bool muted,
                                 bool cameraEnabled,
                                 const QString& role,
                                 const QString& traceId);
QJsonObject buildLivekitLaunchPayload(const QString& livekitUrl, const QString& token, const QJsonObject& metadata);
CallEventPeer resolveCallEventPeer(const QJsonObject& payload, int selfUid);
} // namespace memochat::call_payload

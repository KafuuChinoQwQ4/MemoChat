#include "AppCoordinators.h"

#include "CallCoordinatorPayloadPolicy.h"
#include "CallController.h"

#include <QJsonObject>

void CallCoordinator::onCallEvent(QJsonObject payload)
{
    auto* call = callController();
    if (!call)
    {
        return;
    }

    const QString eventType = payload.value("event_type").toString();
    const QString callId = payload.value("call_id").toString();
    const QString callType = payload.value("call_type").toString();
    const int calleeUid = payload.value("callee_uid").toInt();
    const CallShellSnapshot current = snapshot();
    const int selfUid = current.selfUid;
    const auto peer = memochat::call_payload::resolveCallEventPeer(payload, selfUid);
    const QString peerIcon = _port.normalizeIconPath ? _port.normalizeIconPath(peer.icon) : peer.icon;

    if (eventType == QStringLiteral("call.invite") && calleeUid == selfUid)
    {
        call->startIncoming(callId,
                            callType,
                            peer.name,
                            peerIcon,
                            QStringLiteral("收到来电"),
                                           payload.value("started_at").toVariant().toLongLong(),
                                           payload.value("expires_at").toVariant().toLongLong());
        _port.setAuthStatus(QStringLiteral("%1 邀请你进行%2").arg(
            peer.name,
            callType == QStringLiteral("video") ? QStringLiteral("视频通话") : QStringLiteral("语音通话")), false);
        return;
    }

    if (eventType == QStringLiteral("call.accepted"))
    {
        if (!call->callVisible())
        {
            call->startOutgoing(callId,
                                callType,
                                peer.name,
                                peerIcon,
                                QStringLiteral("已接通"),
                                               payload.value("started_at").toVariant().toLongLong(),
                                               payload.value("expires_at").toVariant().toLongLong());
        }
        call->markAccepted(QStringLiteral("已接通"),
                                          payload.value("room_name").toString(),
                                          payload.value("livekit_url").toString(),
                                          payload.value("accepted_at").toVariant().toLongLong());
        if (selfUid > 0)
        {
            call->fetchToken(selfUid,
                             current.authToken,
                             callId,
                             peer.isCaller
                             ? QStringLiteral("caller")
                             : QStringLiteral("callee"));
        }
        _port.setAuthStatus("通话已接通", false);
        return;
    }

    if (eventType == QStringLiteral("call.rejected") ||
                                    eventType == QStringLiteral("call.cancel") ||
                                                                eventType ==
                                                                    QStringLiteral("call.timeout") ||
                                                                                   eventType ==
                                                                                       QStringLiteral("call.hangup"))
    {
        finalizeEndedCall(memochat::call_payload::callStateText(eventType));
    }
}

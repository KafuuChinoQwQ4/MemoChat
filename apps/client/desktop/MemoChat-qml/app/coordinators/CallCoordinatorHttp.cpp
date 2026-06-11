#include "AppCoordinators.h"

#include "CallCoordinatorPayloadPolicy.h"
#include "CallController.h"
#include "CallSessionModel.h"

#include <QJsonDocument>
#include <QJsonObject>

void CallCoordinator::onCallHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    auto* call = callController();
    if (!call)
    {
        return;
    }

    if (err != ErrorCodes::SUCCESS)
    {
        _port.setAuthStatus("通话服务请求失败", true);
        return;
    }

    QJsonObject obj;
    if (!_port.parseJson || !_port.parseJson(res, obj))
    {
        _port.setAuthStatus("通话服务响应异常", true);
        return;
    }

    const int errorCode = obj.value("error").toInt();
    if (errorCode != ErrorCodes::SUCCESS)
    {
        _port.setAuthStatus(memochat::call_payload::mapCallError(errorCode), true);
        return;
    }

    const CallShellSnapshot current = snapshot();

    if (id == ReqId::ID_CALL_START)
    {
        call->startOutgoing(obj.value("call_id").toString(),
                            obj.value("call_type").toString(),
                            obj.value("peer_name").toString(current.currentContactName),
                            _port.normalizeIconPath
                                ? _port.normalizeIconPath(obj.value("peer_icon").toString(current.currentContactIcon))
                                : obj.value("peer_icon").toString(current.currentContactIcon),
                            QStringLiteral("等待对方接听"),
                                           obj.value("started_at").toVariant().toLongLong(),
                                           obj.value("expires_at").toVariant().toLongLong());
        _port.setAuthStatus("通话邀请已发出", false);
        return;
    }

    if (id == ReqId::ID_CALL_ACCEPT)
    {
        call->markAccepted(QStringLiteral("已接通"),
                                          obj.value("room_name").toString(),
                                          obj.value("livekit_url").toString(),
                                          obj.value("accepted_at").toVariant().toLongLong());
        if (current.selfUid > 0)
        {
            call->fetchToken(current.selfUid,
                             current.authToken,
                             obj.value("call_id").toString(),
                             QStringLiteral("callee"));
        }
        _port.setAuthStatus("正在准备媒体通道", false);
        return;
    }

    if (id == ReqId::ID_CALL_GET_TOKEN)
    {
        const QString livekitUrl = obj.value("livekit_url").toString();
        const QString roomName = obj.value("room_name").toString();
        const QString token = obj.value("token").toString();
        if (livekitUrl.isEmpty() || roomName.isEmpty() || token.isEmpty())
        {
            _port.setAuthStatus("通话媒体凭证缺失", true);
            call->setMediaStatusText(QStringLiteral("媒体凭证缺失"));
            return;
        }

        const QJsonObject metadata = memochat::call_payload::buildLivekitMetadata(call->callId(),
                                                                                  roomName,
                                                                                  call->callType(),
                                                                                  call->callSession()->peerName(),
                                                                                  call->callSession()->peerIcon(),
                                                                                  call->muted(),
                                                                                  call->cameraEnabled(),
                                                                                  obj.value("role").toString(),
                                                                                  obj.value("trace_id").toString());
        const QJsonObject launchPayload =
            memochat::call_payload::buildLivekitLaunchPayload(livekitUrl, token, metadata);

        call->setMediaLaunchJson(QString::fromUtf8(QJsonDocument(launchPayload).toJson(QJsonDocument::Compact)));
        call->markTokenReady(QStringLiteral("正在连接 LiveKit 房间"));
        call->requestJoinRoom(livekitUrl,
                              token,
                              QString::fromUtf8(QJsonDocument(metadata).toJson(QJsonDocument::Compact)));
        _port.setAuthStatus("正在连接音视频房间", false);
        return;
    }

    if (id == ReqId::ID_CALL_REJECT || id == ReqId::ID_CALL_CANCEL || id == ReqId::ID_CALL_HANGUP)
    {
        finalizeEndedCall(memochat::call_payload::callStateText(obj.value("event_type").toString()));
    }
}

#include "AppCoordinators.h"

#include "AppController.h"
#include "CallCoordinatorPayloadPolicy.h"
#include "usermgr.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

CallCoordinator::CallCoordinator(AppController& controller)
    : _app(controller)
{
}

bool CallCoordinator::ensureCallTargetFromCurrentChat()
{
    if (_app.hasCurrentContact())
    {
        return true;
    }
    if (_app._group_state.currentId > 0 || _app._chat_state.uid <= 0)
    {
        return false;
    }
    auto friendInfo = _app._gateway.userMgr()->GetFriendById(_app._chat_state.uid);
    if (!friendInfo)
    {
        return false;
    }
    _app.setCurrentContact(friendInfo->_uid,
                           friendInfo->_name,
                           friendInfo->_nick,
                           friendInfo->_icon,
                           friendInfo->_back,
                           friendInfo->_sex,
                           friendInfo->_user_id);
    return _app.hasCurrentContact();
}

void CallCoordinator::startCallFlow(const QString& callType)
{
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        _app.setAuthStatus("用户状态异常，请重新登录", true);
        return;
    }
    if (_app._contact_state.uid <= 0)
    {
        _app.setAuthStatus("请选择联系人", true);
        return;
    }
    _app._call_controller.startCall(selfInfo->_uid,
                                    _app._gateway.userMgr()->GetToken(),
                                    _app._contact_state.uid,
                                    callType);
    _app.setAuthStatus("正在发起通话", false);
}

void CallCoordinator::startVoiceChat()
{
    if (ensureCallTargetFromCurrentChat())
    {
        startCallFlow("voice");
    }
}

void CallCoordinator::startVideoChat()
{
    if (ensureCallTargetFromCurrentChat())
    {
        startCallFlow("video");
    }
}

void CallCoordinator::acceptIncomingCall()
{
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._call_session_model.callId().isEmpty())
    {
        return;
    }
    _app._call_controller.acceptCall(selfInfo->_uid,
                                     _app._gateway.userMgr()->GetToken(),
                                     _app._call_session_model.callId());
    _app.setAuthStatus("正在接听通话", false);
}

void CallCoordinator::rejectIncomingCall()
{
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._call_session_model.callId().isEmpty())
    {
        return;
    }
    _app._call_controller.rejectCall(selfInfo->_uid,
                                     _app._gateway.userMgr()->GetToken(),
                                     _app._call_session_model.callId());
}

void CallCoordinator::endCurrentCall()
{
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._call_session_model.callId().isEmpty())
    {
        return;
    }
    _app._livekit_bridge.leaveRoom();
    if (!_app._call_session_model.active() && !_app._call_session_model.incoming())
    {
        _app._call_controller.cancelCall(selfInfo->_uid,
                                         _app._gateway.userMgr()->GetToken(),
                                         _app._call_session_model.callId());
        return;
    }
    _app._call_controller.hangupCall(selfInfo->_uid,
                                     _app._gateway.userMgr()->GetToken(),
                                     _app._call_session_model.callId());
}

void CallCoordinator::finalizeEndedCall(const QString& statusText)
{
    _app._livekit_bridge.leaveRoom();
    _app._call_session_model.markEnded(statusText);
    _app.setAuthStatus(statusText, false);
    auto* app = &_app;
    QTimer::singleShot(1800,
                       app,
                       [app]()
                       {
                           app->_call_session_model.clear();
                       });
}

void CallCoordinator::toggleCallMuted()
{
    _app._livekit_bridge.toggleMic();
    _app._call_session_model.setMuted(!_app._call_session_model.muted());
}

void CallCoordinator::toggleCallCamera()
{
    if (_app._call_session_model.callType() != QStringLiteral("video"))
    {
        return;
    }
    _app._livekit_bridge.toggleCamera();
    _app._call_session_model.setCameraEnabled(!_app._call_session_model.cameraEnabled());
}

void CallCoordinator::onCallHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS)
    {
        _app.setAuthStatus("通话服务请求失败", true);
        return;
    }

    QJsonObject obj;
    if (!_app._auth_controller.parseJson(res, obj))
    {
        _app.setAuthStatus("通话服务响应异常", true);
        return;
    }

    const int errorCode = obj.value("error").toInt();
    if (errorCode != ErrorCodes::SUCCESS)
    {
        _app.setAuthStatus(memochat::call_payload::mapCallError(errorCode), true);
        return;
    }

    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    const int selfUid = selfInfo ? selfInfo->_uid : 0;

    if (id == ReqId::ID_CALL_START)
    {
        _app._call_session_model.startOutgoing(
            obj.value("call_id").toString(),
            obj.value("call_type").toString(),
            obj.value("peer_name").toString(_app._contact_state.name),
            _app.normalizeIconPath(obj.value("peer_icon").toString(_app._contact_state.icon)),
            QStringLiteral("等待对方接听"),
                           obj.value("started_at").toVariant().toLongLong(),
                           obj.value("expires_at").toVariant().toLongLong());
        _app.setAuthStatus("通话邀请已发出", false);
        return;
    }

    if (id == ReqId::ID_CALL_ACCEPT)
    {
        _app._call_session_model.markAccepted(QStringLiteral("已接通"),
                                                             obj.value("room_name").toString(),
                                                             obj.value("livekit_url").toString(),
                                                             obj.value("accepted_at").toVariant().toLongLong());
        if (selfUid > 0)
        {
            _app._call_controller.fetchToken(selfUid,
                                             _app._gateway.userMgr()->GetToken(),
                                             obj.value("call_id").toString(),
                                             QStringLiteral("callee"));
        }
        _app.setAuthStatus("正在准备媒体通道", false);
        return;
    }

    if (id == ReqId::ID_CALL_GET_TOKEN)
    {
        const QString livekitUrl = obj.value("livekit_url").toString();
        const QString roomName = obj.value("room_name").toString();
        const QString token = obj.value("token").toString();
        if (livekitUrl.isEmpty() || roomName.isEmpty() || token.isEmpty())
        {
            _app.setAuthStatus("通话媒体凭证缺失", true);
            _app._call_session_model.setMediaStatusText(QStringLiteral("媒体凭证缺失"));
            return;
        }

        const QJsonObject metadata =
            memochat::call_payload::buildLivekitMetadata(_app._call_session_model.callId(),
                                                         roomName,
                                                         _app._call_session_model.callType(),
                                                         _app._call_session_model.peerName(),
                                                         _app._call_session_model.peerIcon(),
                                                         _app._call_session_model.muted(),
                                                         _app._call_session_model.cameraEnabled(),
                                                         obj.value("role").toString(),
                                                         obj.value("trace_id").toString());
        const QJsonObject launchPayload =
            memochat::call_payload::buildLivekitLaunchPayload(livekitUrl, token, metadata);

        _app._call_session_model.setMediaLaunchJson(
            QString::fromUtf8(QJsonDocument(launchPayload).toJson(QJsonDocument::Compact)));
        _app._call_session_model.markTokenReady(QStringLiteral("正在连接 LiveKit 房间"));
        _app._livekit_bridge.requestJoinRoom(livekitUrl,
                                             token,
                                             QString::fromUtf8(QJsonDocument(metadata).toJson(QJsonDocument::Compact)));
        _app.setAuthStatus("正在连接音视频房间", false);
        return;
    }

    if (id == ReqId::ID_CALL_REJECT || id == ReqId::ID_CALL_CANCEL || id == ReqId::ID_CALL_HANGUP)
    {
        finalizeEndedCall(memochat::call_payload::callStateText(obj.value("event_type").toString()));
    }
}

void CallCoordinator::onCallEvent(QJsonObject payload)
{
    const QString eventType = payload.value("event_type").toString();
    const QString callId = payload.value("call_id").toString();
    const QString callType = payload.value("call_type").toString();
    const int calleeUid = payload.value("callee_uid").toInt();
    const int selfUid = _app._gateway.userMgr()->GetUid();
    const auto peer = memochat::call_payload::resolveCallEventPeer(payload, selfUid);
    const QString peerIcon = _app.normalizeIconPath(peer.icon);

    if (eventType == QStringLiteral("call.invite") && calleeUid == selfUid)
    {
        _app._call_session_model.startIncoming(callId,
                                               callType,
                                               peer.name,
                                               peerIcon,
                                               QStringLiteral("收到来电"),
                                                              payload.value("started_at").toVariant().toLongLong(),
                                                              payload.value("expires_at").toVariant().toLongLong());
        _app.setAuthStatus(QStringLiteral("%1 邀请你进行%2").arg(
            peer.name,
            callType == QStringLiteral("video") ? QStringLiteral("视频通话") : QStringLiteral("语音通话")), false);
        return;
    }

    if (eventType == QStringLiteral("call.accepted"))
    {
        if (!_app._call_session_model.visible())
        {
            _app._call_session_model.startOutgoing(callId,
                                                   callType,
                                                   peer.name,
                                                   peerIcon,
                                                   QStringLiteral("已接通"),
                                                                  payload.value("started_at").toVariant().toLongLong(),
                                                                  payload.value("expires_at").toVariant().toLongLong());
        }
        _app._call_session_model.markAccepted(QStringLiteral("已接通"),
                                                             payload.value("room_name").toString(),
                                                             payload.value("livekit_url").toString(),
                                                             payload.value("accepted_at").toVariant().toLongLong());
        if (selfUid > 0)
        {
            _app._call_controller.fetchToken(selfUid,
                                             _app._gateway.userMgr()->GetToken(),
                                             callId,
                                             peer.isCaller
                                             ? QStringLiteral("caller")
                                             : QStringLiteral("callee"));
        }
        _app.setAuthStatus("通话已接通", false);
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

void CallCoordinator::onLivekitRoomJoined()
{
    if (!_app._call_session_model.visible())
    {
        return;
    }
    _app._call_session_model.setMediaStatusText(QStringLiteral("LiveKit 房间已连接"));
    _app.setAuthStatus(QStringLiteral("音视频通道已建立"), false);
}

void CallCoordinator::onLivekitRemoteTrackReady()
{
    if (!_app._call_session_model.visible())
    {
        return;
    }
    _app._call_session_model.setMediaStatusText(QStringLiteral("对端媒体已就绪"));
}

void CallCoordinator::onLivekitRoomDisconnected(const QString& reason, bool recoverable)
{
    if (!_app._call_session_model.visible())
    {
        return;
    }
    if (reason == QStringLiteral("local_leave"))
    {
        return;
    }
    const QString status =
        recoverable ? QStringLiteral("网络波动，正在恢复音视频连接") : QStringLiteral("音视频连接已断开");
    _app._call_session_model.setMediaStatusText(reason.isEmpty() ? status : status + QStringLiteral("：") + reason);
    _app.setAuthStatus(status, !recoverable);
    if (!recoverable)
    {
        finalizeEndedCall(status);
    }
}

void CallCoordinator::onLivekitPermissionError(const QString& deviceType, const QString& message)
{
    Q_UNUSED(deviceType);
    const QString status = message.isEmpty() ? QStringLiteral("麦克风或摄像头权限被拒绝") : message;
    _app._call_session_model.setMediaStatusText(status);
    _app.setAuthStatus(status, true);
    finalizeEndedCall(QStringLiteral("通话未建立"));
}

void CallCoordinator::onLivekitMediaError(const QString& message)
{
    const QString status = message.isEmpty() ? QStringLiteral("音视频媒体初始化失败") : message;
    _app._call_session_model.setMediaStatusText(status);
    _app.setAuthStatus(status, true);
    finalizeEndedCall(QStringLiteral("通话异常结束"));
}

void CallCoordinator::onLivekitReconnecting(const QString& message)
{
    if (!_app._call_session_model.visible())
    {
        return;
    }
    const QString status = message.isEmpty() ? QStringLiteral("网络波动，正在重连") : message;
    _app._call_session_model.setMediaStatusText(status);
    _app.setAuthStatus(status, false);
}

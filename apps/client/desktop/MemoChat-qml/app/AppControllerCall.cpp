#include "AppController.h"

#include "usermgr.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

namespace {
QString mapCallError(int errorCode)
{
    switch (errorCode) {
    case 1015: return QStringLiteral("当前线路忙，请稍后重试");
    case 1016: return QStringLiteral("通话不存在或已失效");
    case 1017: return QStringLiteral("当前通话状态不可执行该操作");
    case 1018: return QStringLiteral("无权执行该通话操作");
    case 1019: return QStringLiteral("对方当前不在线");
    default: return QStringLiteral("通话服务失败");
    }
}

QString callStateText(const QString &eventType)
{
    if (eventType == QStringLiteral("call.invite")) return QStringLiteral("来电中");
    if (eventType == QStringLiteral("call.accepted")) return QStringLiteral("已接通");
    if (eventType == QStringLiteral("call.rejected")) return QStringLiteral("对方已拒绝");
    if (eventType == QStringLiteral("call.cancel")) return QStringLiteral("对方已取消");
    if (eventType == QStringLiteral("call.timeout")) return QStringLiteral("呼叫超时");
    if (eventType == QStringLiteral("call.hangup")) return QStringLiteral("通话结束");
    return QStringLiteral("通话处理中");
}
}

void AppController::startCallFlow(const QString &callType)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setAuthStatus("用户状态异常，请重新登录", true);
        return;
    }
    if (_current_contact_uid <= 0) {
        setAuthStatus("请选择联系人", true);
        return;
    }
    _call_controller.startCall(selfInfo->_uid, _gateway.userMgr()->GetToken(), _current_contact_uid, callType);
    setAuthStatus("正在发起通话", false);
}

void AppController::onCallHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        setAuthStatus("通话服务请求失败", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setAuthStatus("通话服务响应异常", true);
        return;
    }

    const int errorCode = obj.value("error").toInt();
    if (errorCode != ErrorCodes::SUCCESS) {
        setAuthStatus(mapCallError(errorCode), true);
        return;
    }

    const auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const int selfUid = selfInfo ? selfInfo->_uid : 0;

    if (id == ReqId::ID_CALL_START) {
        _call_session_model.startOutgoing(
            obj.value("call_id").toString(),
            obj.value("call_type").toString(),
            obj.value("peer_name").toString(_current_contact_name),
            normalizeIconPath(obj.value("peer_icon").toString(_current_contact_icon)),
            QStringLiteral("等待对方接听"),
            obj.value("started_at").toVariant().toLongLong(),
            obj.value("expires_at").toVariant().toLongLong());
        setAuthStatus("通话邀请已发出", false);
        return;
    }

    if (id == ReqId::ID_CALL_ACCEPT) {
        _call_session_model.markAccepted(QStringLiteral("已接通"),
                                         obj.value("room_name").toString(),
                                         obj.value("livekit_url").toString(),
                                         obj.value("accepted_at").toVariant().toLongLong());
        if (selfUid > 0) {
            _call_controller.fetchToken(selfUid,
                                        _gateway.userMgr()->GetToken(),
                                        obj.value("call_id").toString(),
                                        QStringLiteral("callee"));
        }
        setAuthStatus("正在准备媒体通道", false);
        return;
    }

    if (id == ReqId::ID_CALL_GET_TOKEN) {
        const QString livekitUrl = obj.value("livekit_url").toString();
        const QString roomName = obj.value("room_name").toString();
        const QString token = obj.value("token").toString();
        if (livekitUrl.isEmpty() || roomName.isEmpty() || token.isEmpty()) {
            setAuthStatus("通话媒体凭证缺失", true);
            _call_session_model.setMediaStatusText(QStringLiteral("媒体凭证缺失"));
            return;
        }

        QJsonObject metadata;
        metadata["callId"] = _call_session_model.callId();
        metadata["roomName"] = roomName;
        metadata["callType"] = _call_session_model.callType();
        metadata["peerName"] = _call_session_model.peerName();
        metadata["peerIcon"] = _call_session_model.peerIcon();
        metadata["muted"] = _call_session_model.muted();
        metadata["cameraEnabled"] = _call_session_model.cameraEnabled();
        metadata["role"] = obj.value("role").toString();
        metadata["traceId"] = obj.value("trace_id").toString();

        QJsonObject launchPayload;
        launchPayload["wsUrl"] = livekitUrl;
        launchPayload["token"] = token;
        launchPayload["metadata"] = metadata;

        _call_session_model.setMediaLaunchJson(
            QString::fromUtf8(QJsonDocument(launchPayload).toJson(QJsonDocument::Compact)));
        _call_session_model.markTokenReady(QStringLiteral("正在连接 LiveKit 房间"));
        _livekit_bridge.requestJoinRoom(
            livekitUrl,
            token,
            QString::fromUtf8(QJsonDocument(metadata).toJson(QJsonDocument::Compact)));
        setAuthStatus("正在连接音视频房间", false);
        return;
    }

    if (id == ReqId::ID_CALL_REJECT || id == ReqId::ID_CALL_CANCEL || id == ReqId::ID_CALL_HANGUP) {
        finalizeEndedCall(callStateText(obj.value("event_type").toString()));
    }
}

void AppController::onCallEvent(QJsonObject payload)
{
    const QString eventType = payload.value("event_type").toString();
    const QString callId = payload.value("call_id").toString();
    const QString callType = payload.value("call_type").toString();
    const int callerUid = payload.value("caller_uid").toInt();
    const int calleeUid = payload.value("callee_uid").toInt();
    const int selfUid = _gateway.userMgr()->GetUid();
    const bool isCaller = callerUid == selfUid;
    const QString peerName = isCaller
        ? payload.value("callee_name").toString()
        : payload.value("caller_name").toString();
    const QString peerIcon = normalizeIconPath(isCaller
        ? payload.value("callee_icon").toString()
        : payload.value("caller_icon").toString());

    if (eventType == QStringLiteral("call.invite") && calleeUid == selfUid) {
        _call_session_model.startIncoming(
            callId,
            callType,
            peerName,
            peerIcon,
            QStringLiteral("收到来电"),
            payload.value("started_at").toVariant().toLongLong(),
            payload.value("expires_at").toVariant().toLongLong());
        setAuthStatus(QStringLiteral("%1 邀请你进行%2").arg(
                          peerName,
                          callType == QStringLiteral("video") ? QStringLiteral("视频通话") : QStringLiteral("语音通话")),
                      false);
        return;
    }

    if (eventType == QStringLiteral("call.accepted")) {
        if (!_call_session_model.visible()) {
            _call_session_model.startOutgoing(
                callId, callType, peerName, peerIcon, QStringLiteral("已接通"),
                payload.value("started_at").toVariant().toLongLong(),
                payload.value("expires_at").toVariant().toLongLong());
        }
        _call_session_model.markAccepted(QStringLiteral("已接通"),
                                         payload.value("room_name").toString(),
                                         payload.value("livekit_url").toString(),
                                         payload.value("accepted_at").toVariant().toLongLong());
        if (selfUid > 0) {
            _call_controller.fetchToken(selfUid,
                                        _gateway.userMgr()->GetToken(),
                                        callId,
                                        isCaller ? QStringLiteral("caller") : QStringLiteral("callee"));
        }
        setAuthStatus("通话已接通", false);
        return;
    }

    if (eventType == QStringLiteral("call.rejected") ||
        eventType == QStringLiteral("call.cancel") ||
        eventType == QStringLiteral("call.timeout") ||
        eventType == QStringLiteral("call.hangup")) {
        finalizeEndedCall(callStateText(eventType));
    }
}

void AppController::finalizeEndedCall(const QString &statusText)
{
    _livekit_bridge.leaveRoom();
    _call_session_model.markEnded(statusText);
    setAuthStatus(statusText, false);
    QTimer::singleShot(1800, this, [this]() {
        _call_session_model.clear();
    });
}

void AppController::onLivekitRoomJoined()
{
    if (!_call_session_model.visible()) {
        return;
    }
    _call_session_model.setMediaStatusText(QStringLiteral("LiveKit 房间已连接"));
    setAuthStatus(QStringLiteral("音视频通道已建立"), false);
}

void AppController::onLivekitRemoteTrackReady()
{
    if (!_call_session_model.visible()) {
        return;
    }
    _call_session_model.setMediaStatusText(QStringLiteral("对端媒体已就绪"));
}

void AppController::onLivekitRoomDisconnected(const QString &reason, bool recoverable)
{
    if (!_call_session_model.visible()) {
        return;
    }
    if (reason == QStringLiteral("local_leave")) {
        return;
    }
    const QString status = recoverable
        ? QStringLiteral("网络波动，正在恢复音视频连接")
        : QStringLiteral("音视频连接已断开");
    _call_session_model.setMediaStatusText(reason.isEmpty() ? status : status + QStringLiteral("：") + reason);
    setAuthStatus(status, !recoverable);
    if (!recoverable) {
        finalizeEndedCall(status);
    }
}

void AppController::onLivekitPermissionError(const QString &deviceType, const QString &message)
{
    Q_UNUSED(deviceType);
    const QString status = message.isEmpty()
        ? QStringLiteral("麦克风或摄像头权限被拒绝")
        : message;
    _call_session_model.setMediaStatusText(status);
    setAuthStatus(status, true);
    finalizeEndedCall(QStringLiteral("通话未建立"));
}

void AppController::onLivekitMediaError(const QString &message)
{
    const QString status = message.isEmpty()
        ? QStringLiteral("音视频媒体初始化失败")
        : message;
    _call_session_model.setMediaStatusText(status);
    setAuthStatus(status, true);
    finalizeEndedCall(QStringLiteral("通话异常结束"));
}

void AppController::onLivekitReconnecting(const QString &message)
{
    if (!_call_session_model.visible()) {
        return;
    }
    const QString status = message.isEmpty()
        ? QStringLiteral("网络波动，正在重连")
        : message;
    _call_session_model.setMediaStatusText(status);
    setAuthStatus(status, false);
}

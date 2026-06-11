#include "AppCoordinators.h"

#include "CallController.h"

void CallCoordinator::onLivekitRoomJoined()
{
    auto* call = callController();
    if (!call || !call->callVisible())
    {
        return;
    }
    call->setMediaStatusText(QStringLiteral("LiveKit 房间已连接"));
    _port.setAuthStatus(QStringLiteral("音视频通道已建立"), false);
}

void CallCoordinator::onLivekitRemoteTrackReady()
{
    auto* call = callController();
    if (!call || !call->callVisible())
    {
        return;
    }
    call->setMediaStatusText(QStringLiteral("对端媒体已就绪"));
}

void CallCoordinator::onLivekitRoomDisconnected(const QString& reason, bool recoverable)
{
    auto* call = callController();
    if (!call || !call->callVisible())
    {
        return;
    }
    if (reason == QStringLiteral("local_leave"))
    {
        return;
    }
    const QString status =
        recoverable ? QStringLiteral("网络波动，正在恢复音视频连接") : QStringLiteral("音视频连接已断开");
    call->setMediaStatusText(reason.isEmpty() ? status : status + QStringLiteral("：") + reason);
    _port.setAuthStatus(status, !recoverable);
    if (!recoverable)
    {
        finalizeEndedCall(status);
    }
}

void CallCoordinator::onLivekitPermissionError(const QString& deviceType, const QString& message)
{
    Q_UNUSED(deviceType);
    auto* call = callController();
    if (!call)
    {
        return;
    }
    const QString status = message.isEmpty() ? QStringLiteral("麦克风或摄像头权限被拒绝") : message;
    call->setMediaStatusText(status);
    _port.setAuthStatus(status, true);
    finalizeEndedCall(QStringLiteral("通话未建立"));
}

void CallCoordinator::onLivekitMediaError(const QString& message)
{
    auto* call = callController();
    if (!call)
    {
        return;
    }
    const QString status = message.isEmpty() ? QStringLiteral("音视频媒体初始化失败") : message;
    call->setMediaStatusText(status);
    _port.setAuthStatus(status, true);
    finalizeEndedCall(QStringLiteral("通话异常结束"));
}

void CallCoordinator::onLivekitReconnecting(const QString& message)
{
    auto* call = callController();
    if (!call || !call->callVisible())
    {
        return;
    }
    const QString status = message.isEmpty() ? QStringLiteral("网络波动，正在重连") : message;
    call->setMediaStatusText(status);
    _port.setAuthStatus(status, false);
}

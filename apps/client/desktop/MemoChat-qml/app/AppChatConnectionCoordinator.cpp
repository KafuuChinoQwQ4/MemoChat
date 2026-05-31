#include "AppChatConnectionCoordinator.h"

#include "AppChatConnectionPolicy.h"
#include "AppCoordinators.h"
#include "AppController.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

namespace
{
constexpr int kHeartbeatAckMissThreshold = 2;
}

AppChatConnectionCoordinator::AppChatConnectionCoordinator(AppController& controller)
    : _app(controller)
{
}

void AppChatConnectionCoordinator::onTcpConnectFinished(bool success)
{
    _app._chat_recovery_state.ignoreNextLoginDisconnect = false;
    if (!success)
    {
        _app._chat_login_timeout_timer.stop();
        qWarning() << "Chat transport connect failed. reconnecting:" << _app._chat_recovery_state.reconnecting
                   << "page:" << _app._page << "host:" << _app._chat_endpoint_state.host
                   << "port:" << _app._chat_endpoint_state.port;
        if (_app._chat_recovery_state.reconnecting && _app._page == AppController::ChatPage)
        {
            handleChatConnectFailureDuringRecovery();
            return;
        }
        _app.setBusy(false);
        _app.setTip("聊天服务不可用或连接失败，请确认服务已启动", true);
        return;
    }

    _app._chat_endpoint_state.connectFinishedMs = QDateTime::currentMSecsSinceEpoch();
    qInfo() << "Chat transport connected, sending chat login for uid:" << _app._pending_login_state.uid;
    _app.setTip("聊天服务连接成功，正在登录...", false);
    QJsonObject obj;
    obj["protocol_version"] = _app._chat_endpoint_state.protocolVersion;
    if (_app._chat_endpoint_state.protocolVersion >= 3 && !_app._pending_login_state.loginTicket.isEmpty())
    {
        obj["login_ticket"] = _app._pending_login_state.loginTicket;
    }
    else
    {
        obj["uid"] = _app._pending_login_state.uid;
        obj["token"] = _app._pending_login_state.token;
    }
    if (!_app._pending_login_state.traceId.isEmpty())
    {
        obj["trace_id"] = _app._pending_login_state.traceId;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_CHAT_LOGIN, payload);
    _app._chat_login_timeout_timer.start();
}

void AppChatConnectionCoordinator::onChatLoginFailed(int err)
{
    _app._chat_recovery_state.ignoreNextLoginDisconnect = false;
    _app._chat_login_timeout_timer.stop();
    qWarning() << "Chat login failed, err:" << err << "reconnecting:" << _app._chat_recovery_state.reconnecting
               << "page:" << _app._page;
    if (_app._chat_recovery_state.reconnecting && _app._page == AppController::ChatPage)
    {
        _app._heartbeat_timer.stop();
        _app.switchToLogin();
        _app.setTip("重连失败，会话已失效，请重新登录", true);
        return;
    }
    _app.setBusy(false);
    if (err == ErrorCodes::ERR_PROTOCOL_VERSION)
    {
        _app.setTip("客户端版本过旧，请升级到 v3 协议客户端", true);
        return;
    }
    if (err == ErrorCodes::ERR_CHAT_TICKET_INVALID || err == ErrorCodes::ERR_CHAT_TICKET_EXPIRED ||
        err == ErrorCodes::ERR_CHAT_SERVER_MISMATCH)
    {
        _app.setTip("聊天登录票据已失效，请重新登录", true);
        return;
    }
    _app.setTip(QString("聊天服务登录失败（错误码:%1）").arg(err), true);
}

void AppChatConnectionCoordinator::onConnectionClosed()
{
    qWarning() << "Chat connection closed. page:" << _app._page << "busy:" << _app._shell_state.busy
               << "reconnecting:" << _app._chat_recovery_state.reconnecting
               << "ignore_disconnect:" << _app._chat_recovery_state.ignoreNextLoginDisconnect;
    if (_app._chat_recovery_state.ignoreNextLoginDisconnect)
    {
        _app._chat_login_timeout_timer.stop();
        _app._chat_recovery_state.ignoreNextLoginDisconnect = false;
        resetReconnectState();
        resetHeartbeatTracking();
        return;
    }

    if (_app._page != AppController::ChatPage)
    {
        if (_app._shell_state.busy)
        {
            if (_app._chat_login_timeout_timer.isActive())
            {
                _app._chat_login_timeout_timer.stop();
                _app._chat_recovery_state.ignoreNextLoginDisconnect = false;
                if (tryLoginFallbackToTcp(QStringLiteral("connection_closed")))
                {
                    return;
                }
                _app.setBusy(false);
                _app.setTip("聊天连接已断开，登录已取消，请重试", true);
            }
            resetReconnectState();
            resetHeartbeatTracking();
            return;
        }

        _app._chat_login_timeout_timer.stop();
        resetReconnectState();
        resetHeartbeatTracking();
        return;
    }

    if (_app._call_session_model.visible())
    {
        _app._call_coordinator->finalizeEndedCall(QStringLiteral("通话链路已断开"));
    }

    _app._chat_login_timeout_timer.stop();
    _app._heartbeat_timer.stop();
    if (tryReconnectChat())
    {
        return;
    }
    const bool heartbeatTimeout = isHeartbeatLikelyTimeout();
    qWarning() << "Chat reconnect exhausted. heartbeat timeout:" << heartbeatTimeout;
    _app.switchToLogin();
    _app.setTip(heartbeatTimeout ? "心跳超时，请重新登录" : "聊天连接已断开，请重新登录", true);
}

void AppChatConnectionCoordinator::onHeartbeatTimeout()
{
    if (_app._page != AppController::ChatPage)
    {
        return;
    }

    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (_app._chat_recovery_state.lastHeartbeatSentMs > 0 &&
        _app._chat_recovery_state.lastHeartbeatAckMs < _app._chat_recovery_state.lastHeartbeatSentMs)
    {
        ++_app._chat_recovery_state.heartbeatAckMissCount;
    }
    else
    {
        _app._chat_recovery_state.heartbeatAckMissCount = 0;
    }

    if (_app._chat_recovery_state.heartbeatAckMissCount >= kHeartbeatAckMissThreshold &&
        (nowMs - _app._chat_recovery_state.lastHeartbeatSentMs) >= AppChatConnectionPolicy::kHeartbeatAckGraceMs)
    {
        _app._chat_recovery_state.closedByHeartbeatWatchdog = true;
        _app._heartbeat_timer.stop();
        _app._gateway.chatTransport()->CloseConnection();
        return;
    }

    QJsonObject hb;
    hb["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(hb).toJson(QJsonDocument::Compact);
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_HEART_BEAT_REQ, payload);
    _app._chat_recovery_state.lastHeartbeatSentMs = nowMs;
}

void AppChatConnectionCoordinator::onHeartbeatAck(qint64 ackAtMs)
{
    if (ackAtMs <= 0)
    {
        ackAtMs = QDateTime::currentMSecsSinceEpoch();
    }
    _app._chat_recovery_state.lastHeartbeatAckMs = ackAtMs;
    _app._chat_recovery_state.heartbeatAckMissCount = 0;
}

void AppChatConnectionCoordinator::onNotifyOffline()
{
    _app._chat_login_timeout_timer.stop();
    if (_app._page != AppController::ChatPage)
    {
        return;
    }

    qWarning() << "Received offline notification for current session.";
    _app._heartbeat_timer.stop();
    resetReconnectState();
    resetHeartbeatTracking();
    _app._gateway.chatTransport()->CloseConnection();
    _app.switchToLogin();
    _app.setTip("同账号异地登录，该终端下线", true);
}

bool AppChatConnectionCoordinator::tryLoginFallbackToTcp(const QString& reason)
{
    AppChatConnectionPolicy::AppChatConnectionSnapshot snapshot;
    snapshot.isLoginPage = _app._page == AppController::LoginPage;
    snapshot.busy = _app._shell_state.busy;
    snapshot.loginTcpFallbackAttempted = _app._chat_recovery_state.loginTcpFallbackAttempted;
    snapshot.uid = _app._pending_login_state.uid;
    snapshot.token = _app._pending_login_state.token;
    snapshot.loginTicket = _app._pending_login_state.loginTicket;
    snapshot.endpoints = _app._chat_endpoint_state.endpoints;
    snapshot.connectTimeoutMs = _app._chat_endpoint_state.connectTimeoutMs;
    snapshot.backupDialDelayMs = _app._chat_endpoint_state.backupDialDelayMs;
    snapshot.totalLoginTimeoutMs = _app._chat_endpoint_state.totalLoginTimeoutMs;
    snapshot.protocolVersion = _app._chat_endpoint_state.protocolVersion;

    const AppChatConnectionPolicy::AppChatConnectionDecision decision =
        AppChatConnectionPolicy::evaluateLoginTcpFallback(snapshot);
    if (!decision.allowed)
    {
        return false;
    }

    _app._chat_endpoint_state.host = decision.selectedEndpoint.host;
    _app._chat_endpoint_state.port = decision.selectedEndpoint.port;
    _app._chat_endpoint_state.serverName = decision.selectedEndpoint.serverName;

    qWarning() << "Chat login over current transport failed; falling back to tcp."
               << "reason:" << reason << "host:" << decision.serverInfo.Host << "port:" << decision.serverInfo.Port;
    _app._chat_recovery_state.loginTcpFallbackAttempted = true;
    _app.setTip("QUIC 登录未完成，正在回退到 TCP 长连接...", false);
    _app._chat_endpoint_state.connectStartedMs = QDateTime::currentMSecsSinceEpoch();
    _app._gateway.chatTransport()->connectToServer(decision.serverInfo);
    return true;
}

bool AppChatConnectionCoordinator::tryReconnectChat()
{
    AppChatConnectionPolicy::AppChatConnectionSnapshot snapshot;
    snapshot.isChatPage = _app._page == AppController::ChatPage;
    snapshot.reconnectAttempts = _app._chat_recovery_state.reconnectAttempts;
    snapshot.uid = _app._pending_login_state.uid;
    snapshot.token = _app._pending_login_state.token;
    snapshot.loginTicket = _app._pending_login_state.loginTicket;
    snapshot.endpoints = _app._chat_endpoint_state.endpoints;
    snapshot.connectTimeoutMs = _app._chat_endpoint_state.connectTimeoutMs;
    snapshot.backupDialDelayMs = _app._chat_endpoint_state.backupDialDelayMs;
    snapshot.totalLoginTimeoutMs = _app._chat_endpoint_state.totalLoginTimeoutMs;
    snapshot.protocolVersion = _app._chat_endpoint_state.protocolVersion;

    const AppChatConnectionPolicy::AppChatConnectionDecision decision =
        AppChatConnectionPolicy::evaluateReconnect(snapshot);
    if (!decision.allowed)
    {
        if (_app._page == AppController::ChatPage &&
            _app._chat_recovery_state.reconnectAttempts >= AppChatConnectionPolicy::kChatReconnectMaxAttempts)
        {
            qWarning() << "Reconnect attempts exhausted for uid:" << _app._pending_login_state.uid
                       << "host:" << _app._chat_endpoint_state.host << "port:" << _app._chat_endpoint_state.port;
        }
        return false;
    }

    _app._chat_recovery_state.reconnecting = true;
    ++_app._chat_recovery_state.reconnectAttempts;
    qInfo() << "Attempting chat reconnect" << _app._chat_recovery_state.reconnectAttempts << "/"
            << AppChatConnectionPolicy::kChatReconnectMaxAttempts << "uid:" << _app._pending_login_state.uid
            << "host:" << _app._chat_endpoint_state.host << "port:" << _app._chat_endpoint_state.port;
    _app.setTip(QString("聊天连接断开，正在重连（%1/%2）...")
                    .arg(_app._chat_recovery_state.reconnectAttempts)
                    .arg(AppChatConnectionPolicy::kChatReconnectMaxAttempts),
                true);

    _app._chat_endpoint_state.endpointIndex = 0;
    _app._chat_endpoint_state.host = decision.selectedEndpoint.host;
    _app._chat_endpoint_state.port = decision.selectedEndpoint.port;
    _app._chat_endpoint_state.serverName = decision.selectedEndpoint.serverName;

    QTimer::singleShot(AppChatConnectionPolicy::kChatReconnectDelayMs,
                       &_app,
                       [this, serverInfo = decision.serverInfo]()
                       {
                           if (!_app._chat_recovery_state.reconnecting || _app._page != AppController::ChatPage)
                           {
                               return;
                           }
                           _app._chat_endpoint_state.connectStartedMs = QDateTime::currentMSecsSinceEpoch();
                           _app._gateway.chatTransport()->connectToServer(serverInfo);
                       });
    return true;
}

void AppChatConnectionCoordinator::handleChatConnectFailureDuringRecovery()
{
    if (tryReconnectChat())
    {
        return;
    }
    _app._heartbeat_timer.stop();
    _app.switchToLogin();
    _app.setTip("聊天重连失败，请重新登录", true);
}

void AppChatConnectionCoordinator::resetReconnectState()
{
    _app._chat_recovery_state.reconnecting = false;
    _app._chat_recovery_state.reconnectAttempts = 0;
}

void AppChatConnectionCoordinator::resetHeartbeatTracking()
{
    _app._chat_recovery_state.lastHeartbeatSentMs = 0;
    _app._chat_recovery_state.lastHeartbeatAckMs = 0;
    _app._chat_recovery_state.heartbeatAckMissCount = 0;
    _app._chat_recovery_state.closedByHeartbeatWatchdog = false;
}

bool AppChatConnectionCoordinator::isHeartbeatLikelyTimeout() const
{
    if (_app._chat_recovery_state.closedByHeartbeatWatchdog)
    {
        return true;
    }
    if (_app._chat_recovery_state.lastHeartbeatSentMs <= 0)
    {
        return false;
    }
    if (_app._chat_recovery_state.lastHeartbeatAckMs >= _app._chat_recovery_state.lastHeartbeatSentMs)
    {
        return false;
    }
    return (QDateTime::currentMSecsSinceEpoch() - _app._chat_recovery_state.lastHeartbeatSentMs) >=
           AppChatConnectionPolicy::kHeartbeatAckGraceMs;
}

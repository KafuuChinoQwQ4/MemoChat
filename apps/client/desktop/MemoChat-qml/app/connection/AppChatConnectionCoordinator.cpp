#include "AppChatConnectionCoordinator.h"

#include "AppChatConnectionPolicy.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>

namespace
{
constexpr int kHeartbeatAckMissThreshold = 2;

AppChatConnectionPolicy::AppChatConnectionSnapshot toPolicySnapshot(const ChatConnectionSnapshot& snapshot)
{
    AppChatConnectionPolicy::AppChatConnectionSnapshot policySnapshot;
    policySnapshot.isLoginPage = snapshot.isLoginPage;
    policySnapshot.isChatPage = snapshot.isChatPage;
    policySnapshot.busy = snapshot.busy;
    policySnapshot.loginTcpFallbackAttempted = snapshot.loginTcpFallbackAttempted;
    policySnapshot.reconnectAttempts = snapshot.reconnectAttempts;
    policySnapshot.uid = snapshot.uid;
    policySnapshot.token = snapshot.token;
    policySnapshot.loginTicket = snapshot.loginTicket;
    policySnapshot.endpoints = snapshot.endpoints;
    policySnapshot.connectTimeoutMs = snapshot.connectTimeoutMs;
    policySnapshot.backupDialDelayMs = snapshot.backupDialDelayMs;
    policySnapshot.totalLoginTimeoutMs = snapshot.totalLoginTimeoutMs;
    policySnapshot.protocolVersion = snapshot.protocolVersion;
    return policySnapshot;
}
} // namespace

AppChatConnectionCoordinator::AppChatConnectionCoordinator(ChatConnectionPort port)
    : _port(std::move(port))
{
}

void AppChatConnectionCoordinator::onTcpConnectFinished(bool success)
{
    _port.setIgnoreNextLoginDisconnect(false);
    const ChatConnectionSnapshot snapshot = _port.snapshot();
    if (!success)
    {
        _port.stopLoginTimeoutTimer();
        qWarning() << "Chat transport connect failed. reconnecting:" << snapshot.reconnecting
                   << "page:" << snapshot.page << "host:" << snapshot.host << "port:" << snapshot.port;
        if (snapshot.reconnecting && snapshot.isChatPage)
        {
            handleChatConnectFailureDuringRecovery();
            return;
        }
        _port.setBusy(false);
        _port.setTip("聊天服务不可用或连接失败，请确认服务已启动", true);
        return;
    }

    const qint64 connectFinishedMs = QDateTime::currentMSecsSinceEpoch();
    _port.setConnectFinishedMs(connectFinishedMs);
    qInfo() << "Chat transport connected, sending chat login for uid:" << snapshot.uid;
    _port.setTip("聊天服务连接成功，正在登录...", false);
    QJsonObject obj;
    obj["protocol_version"] = snapshot.protocolVersion;
    if (snapshot.protocolVersion >= 3 && !snapshot.loginTicket.isEmpty())
    {
        obj["login_ticket"] = snapshot.loginTicket;
    }
    else
    {
        obj["uid"] = snapshot.uid;
        obj["token"] = snapshot.token;
    }
    if (!snapshot.traceId.isEmpty())
    {
        obj["trace_id"] = snapshot.traceId;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _port.sendChatLogin(payload);
    _port.startLoginTimeoutTimer();
}

void AppChatConnectionCoordinator::onChatLoginFailed(int err)
{
    _port.setIgnoreNextLoginDisconnect(false);
    _port.stopLoginTimeoutTimer();
    const ChatConnectionSnapshot snapshot = _port.snapshot();
    qWarning() << "Chat login failed, err:" << err << "reconnecting:" << snapshot.reconnecting
               << "page:" << snapshot.page;
    if (snapshot.reconnecting && snapshot.isChatPage)
    {
        _port.stopHeartbeatTimer();
        _port.switchToLogin();
        _port.setTip("重连失败，会话已失效，请重新登录", true);
        return;
    }
    _port.setBusy(false);
    if (err == ErrorCodes::ERR_PROTOCOL_VERSION)
    {
        _port.setTip("客户端版本过旧，请升级到 v3 协议客户端", true);
        return;
    }
    if (err == ErrorCodes::ERR_CHAT_TICKET_INVALID || err == ErrorCodes::ERR_CHAT_TICKET_EXPIRED ||
        err == ErrorCodes::ERR_CHAT_SERVER_MISMATCH)
    {
        _port.setTip("聊天登录票据已失效，请重新登录", true);
        return;
    }
    _port.setTip(QString("聊天服务登录失败（错误码:%1）").arg(err), true);
}

void AppChatConnectionCoordinator::onConnectionClosed()
{
    const ChatConnectionSnapshot snapshot = _port.snapshot();
    qWarning() << "Chat connection closed. page:" << snapshot.page << "busy:" << snapshot.busy
               << "reconnecting:" << snapshot.reconnecting
               << "ignore_disconnect:" << snapshot.ignoreNextLoginDisconnect;
    if (snapshot.ignoreNextLoginDisconnect)
    {
        _port.stopLoginTimeoutTimer();
        _port.setIgnoreNextLoginDisconnect(false);
        resetReconnectState();
        resetHeartbeatTracking();
        return;
    }

    if (!snapshot.isChatPage)
    {
        if (snapshot.busy)
        {
            if (_port.loginTimeoutTimerActive())
            {
                _port.stopLoginTimeoutTimer();
                _port.setIgnoreNextLoginDisconnect(false);
                if (tryLoginFallbackToTcp(QStringLiteral("connection_closed")))
                {
                    return;
                }
                _port.setBusy(false);
                _port.setTip("聊天连接已断开，登录已取消，请重试", true);
            }
            resetReconnectState();
            resetHeartbeatTracking();
            return;
        }

        _port.stopLoginTimeoutTimer();
        resetReconnectState();
        resetHeartbeatTracking();
        return;
    }

    if (_port.callVisible())
    {
        _port.finalizeEndedCall(QStringLiteral("通话链路已断开"));
    }

    _port.stopLoginTimeoutTimer();
    _port.stopHeartbeatTimer();
    if (tryReconnectChat())
    {
        return;
    }
    const bool heartbeatTimeout = isHeartbeatLikelyTimeout();
    qWarning() << "Chat reconnect exhausted. heartbeat timeout:" << heartbeatTimeout;
    _port.switchToLogin();
    _port.setTip(heartbeatTimeout ? "心跳超时，请重新登录" : "聊天连接已断开，请重新登录", true);
}

void AppChatConnectionCoordinator::onLoginTimeout()
{
    const ChatConnectionSnapshot snapshot = _port.snapshot();
    if (snapshot.reconnecting && snapshot.isChatPage)
    {
        _port.closeConnection();
        return;
    }
    if (!snapshot.busy || !snapshot.isLoginPage)
    {
        return;
    }

    const bool fallbackAlreadyAttempted = snapshot.loginTcpFallbackAttempted;
    _port.closeConnection();
    if (!fallbackAlreadyAttempted && _port.snapshot().loginTcpFallbackAttempted)
    {
        return;
    }
    if (tryLoginFallbackToTcp(QStringLiteral("login_timeout")))
    {
        return;
    }
    _port.setBusy(false);
    _port.setTip("聊天服务登录超时，请重试", true);
}

void AppChatConnectionCoordinator::onHeartbeatTimeout()
{
    const ChatConnectionSnapshot snapshot = _port.snapshot();
    if (!snapshot.isChatPage)
    {
        return;
    }

    const int selfUid = _port.currentUserUid();
    if (selfUid <= 0)
    {
        return;
    }

    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    int missCount = 0;
    if (snapshot.lastHeartbeatSentMs > 0 && snapshot.lastHeartbeatAckMs < snapshot.lastHeartbeatSentMs)
    {
        missCount = snapshot.heartbeatAckMissCount + 1;
    }
    _port.setHeartbeatAckMissCount(missCount);

    if (missCount >= kHeartbeatAckMissThreshold &&
        (nowMs - snapshot.lastHeartbeatSentMs) >= AppChatConnectionPolicy::kHeartbeatAckGraceMs)
    {
        _port.setClosedByHeartbeatWatchdog(true);
        _port.stopHeartbeatTimer();
        _port.closeConnection();
        return;
    }

    QJsonObject hb;
    hb["fromuid"] = selfUid;
    const QByteArray payload = QJsonDocument(hb).toJson(QJsonDocument::Compact);
    _port.sendHeartbeat(payload);
    _port.setLastHeartbeatSentMs(nowMs);
}

void AppChatConnectionCoordinator::onHeartbeatAck(qint64 ackAtMs)
{
    if (ackAtMs <= 0)
    {
        ackAtMs = QDateTime::currentMSecsSinceEpoch();
    }
    _port.setLastHeartbeatAckMs(ackAtMs);
    _port.setHeartbeatAckMissCount(0);
}

void AppChatConnectionCoordinator::onNotifyOffline()
{
    _port.stopLoginTimeoutTimer();
    const ChatConnectionSnapshot snapshot = _port.snapshot();
    if (!snapshot.isChatPage)
    {
        return;
    }

    qWarning() << "Received offline notification for current session.";
    _port.stopHeartbeatTimer();
    resetReconnectState();
    resetHeartbeatTracking();
    _port.closeConnection();
    _port.switchToLogin();
    _port.setTip("同账号异地登录，该终端下线", true);
}

bool AppChatConnectionCoordinator::tryLoginFallbackToTcp(const QString& reason)
{
    const ChatConnectionSnapshot snapshot = _port.snapshot();
    const AppChatConnectionPolicy::AppChatConnectionDecision decision =
        AppChatConnectionPolicy::evaluateLoginTcpFallback(toPolicySnapshot(snapshot));
    if (!decision.allowed)
    {
        return false;
    }

    _port.setCurrentEndpoint(decision.selectedEndpoint);

    qWarning() << "Chat login over current transport failed; falling back to tcp."
               << "reason:" << reason << "host:" << decision.serverInfo.Host << "port:" << decision.serverInfo.Port;
    _port.setLoginTcpFallbackAttempted(true);
    _port.setTip("QUIC 登录未完成，正在回退到 TCP 长连接...", false);
    _port.setConnectStartedMs(QDateTime::currentMSecsSinceEpoch());
    _port.connectToServer(decision.serverInfo);
    return true;
}

bool AppChatConnectionCoordinator::tryReconnectChat()
{
    const ChatConnectionSnapshot snapshot = _port.snapshot();
    const AppChatConnectionPolicy::AppChatConnectionDecision decision =
        AppChatConnectionPolicy::evaluateReconnect(toPolicySnapshot(snapshot));
    if (!decision.allowed)
    {
        if (snapshot.isChatPage && snapshot.reconnectAttempts >= AppChatConnectionPolicy::kChatReconnectMaxAttempts)
        {
            qWarning() << "Reconnect attempts exhausted for uid:" << snapshot.uid << "host:" << snapshot.host
                       << "port:" << snapshot.port;
        }
        return false;
    }

    const int nextReconnectAttempt = snapshot.reconnectAttempts + 1;
    _port.setReconnecting(true);
    _port.setReconnectAttempts(nextReconnectAttempt);
    qInfo() << "Attempting chat reconnect" << nextReconnectAttempt << "/"
            << AppChatConnectionPolicy::kChatReconnectMaxAttempts << "uid:" << snapshot.uid << "host:" << snapshot.host
            << "port:" << snapshot.port;
    _port.setTip(QString("聊天连接断开，正在重连（%1/%2）...")
                     .arg(nextReconnectAttempt)
                     .arg(AppChatConnectionPolicy::kChatReconnectMaxAttempts),
                 true);

    _port.setEndpointIndex(0);
    _port.setCurrentEndpoint(decision.selectedEndpoint);

    _port.runDelayedReconnect(AppChatConnectionPolicy::kChatReconnectDelayMs,
                              [this, serverInfo = decision.serverInfo]()
                              {
                                  const ChatConnectionSnapshot current = _port.snapshot();
                                  if (!current.reconnecting || !current.isChatPage)
                                  {
                                      return;
                                  }
                                  _port.setConnectStartedMs(QDateTime::currentMSecsSinceEpoch());
                                  _port.connectToServer(serverInfo);
                              });
    return true;
}

void AppChatConnectionCoordinator::handleChatConnectFailureDuringRecovery()
{
    if (tryReconnectChat())
    {
        return;
    }
    _port.stopHeartbeatTimer();
    _port.switchToLogin();
    _port.setTip("聊天重连失败，请重新登录", true);
}

void AppChatConnectionCoordinator::resetReconnectState()
{
    _port.setReconnecting(false);
    _port.setReconnectAttempts(0);
}

void AppChatConnectionCoordinator::resetHeartbeatTracking()
{
    _port.setLastHeartbeatSentMs(0);
    _port.setLastHeartbeatAckMs(0);
    _port.setHeartbeatAckMissCount(0);
    _port.setClosedByHeartbeatWatchdog(false);
}

bool AppChatConnectionCoordinator::isHeartbeatLikelyTimeout() const
{
    const ChatConnectionSnapshot snapshot = _port.snapshot();
    if (snapshot.closedByHeartbeatWatchdog)
    {
        return true;
    }
    if (snapshot.lastHeartbeatSentMs <= 0)
    {
        return false;
    }
    if (snapshot.lastHeartbeatAckMs >= snapshot.lastHeartbeatSentMs)
    {
        return false;
    }
    return (QDateTime::currentMSecsSinceEpoch() - snapshot.lastHeartbeatSentMs) >=
           AppChatConnectionPolicy::kHeartbeatAckGraceMs;
}

#include "AppController.h"
#include "IChatTransport.h"

#include <QDateTime>
#include <QTimer>
#include <QDebug>

namespace
{
constexpr qint64 kHeartbeatAckGraceMs = 15000;
constexpr int kChatReconnectMaxAttempts = 2;
constexpr int kChatReconnectDelayMs = 300;

bool hasEndpointKind(const QVector<ChatEndpoint>& endpoints, ChatTransportKind kind)
{
    for (const auto& endpoint : endpoints)
    {
        if (endpoint.transport == kind)
        {
            return true;
        }
    }
    return false;
}
} // namespace

void AppController::onConnectionClosed()
{
    qWarning() << "Chat connection closed. page:" << _page << "busy:" << _shell_state.busy
               << "reconnecting:" << _chat_recovery_state.reconnecting
               << "ignore_disconnect:" << _chat_recovery_state.ignoreNextLoginDisconnect;
    if (_chat_recovery_state.ignoreNextLoginDisconnect)
    {
        _chat_login_timeout_timer.stop();
        _chat_recovery_state.ignoreNextLoginDisconnect = false;
        resetReconnectState();
        resetHeartbeatTracking();
        return;
    }

    if (_page != ChatPage)
    {
        if (_shell_state.busy)
        {
            if (_chat_login_timeout_timer.isActive())
            {
                _chat_login_timeout_timer.stop();
                _chat_recovery_state.ignoreNextLoginDisconnect = false;
                if (tryLoginFallbackToTcp(QStringLiteral("connection_closed")))
                {
                    return;
                }
                setBusy(false);
                setTip("聊天连接已断开，登录已取消，请重试", true);
            }
            resetReconnectState();
            resetHeartbeatTracking();
            return;
        }

        _chat_login_timeout_timer.stop();
        resetReconnectState();
        resetHeartbeatTracking();
        return;
    }

    if (_call_session_model.visible())
    {
        finalizeEndedCall(QStringLiteral("通话链路已断开"));
    }

    _chat_login_timeout_timer.stop();
    _heartbeat_timer.stop();
    if (tryReconnectChat())
    {
        return;
    }
    const bool heartbeatTimeout = isHeartbeatLikelyTimeout();
    qWarning() << "Chat reconnect exhausted. heartbeat timeout:" << heartbeatTimeout;
    switchToLogin();
    setTip(heartbeatTimeout ? "心跳超时，请重新登录" : "聊天连接已断开，请重新登录", true);
}

bool AppController::tryLoginFallbackToTcp(const QString& reason)
{
    if (_chat_recovery_state.loginTcpFallbackAttempted)
    {
        return false;
    }
    if (_page != LoginPage || !_shell_state.busy)
    {
        return false;
    }
    if (!hasEndpointKind(_chat_endpoint_state.endpoints, ChatTransportKind::Tcp))
    {
        return false;
    }
    const bool missingCredential = _chat_endpoint_state.protocolVersion >= 3
                                       ? _pending_login_state.loginTicket.isEmpty()
                                       : _pending_login_state.token.isEmpty();
    if (_pending_login_state.uid <= 0 || missingCredential)
    {
        return false;
    }

    ServerInfo serverInfo;
    serverInfo.Uid = _pending_login_state.uid;
    serverInfo.Token = _pending_login_state.token;
    serverInfo.LoginTicket = _pending_login_state.loginTicket;
    serverInfo.ProtocolVersion = _chat_endpoint_state.protocolVersion;
    serverInfo.Endpoints = _chat_endpoint_state.endpoints;
    serverInfo.PreferredTransport = ChatTransportKind::Tcp;
    serverInfo.FallbackTransport = ChatTransportKind::Tcp;
    serverInfo.ConnectTimeoutMs = _chat_endpoint_state.connectTimeoutMs;
    serverInfo.BackupDialDelayMs = _chat_endpoint_state.backupDialDelayMs;
    serverInfo.TotalLoginTimeoutMs = _chat_endpoint_state.totalLoginTimeoutMs;
    for (const auto& endpoint : _chat_endpoint_state.endpoints)
    {
        if (endpoint.transport == ChatTransportKind::Tcp)
        {
            _chat_endpoint_state.host = endpoint.host;
            _chat_endpoint_state.port = endpoint.port;
            _chat_endpoint_state.serverName = endpoint.serverName;
            serverInfo.Host = endpoint.host;
            serverInfo.Port = endpoint.port;
            serverInfo.ServerName = endpoint.serverName;
            break;
        }
    }
    if (serverInfo.Host.trimmed().isEmpty() || serverInfo.Port.trimmed().isEmpty())
    {
        return false;
    }

    qWarning() << "Chat login over current transport failed; falling back to tcp."
               << "reason:" << reason << "host:" << serverInfo.Host << "port:" << serverInfo.Port;
    _chat_recovery_state.loginTcpFallbackAttempted = true;
    setTip("QUIC 登录未完成，正在回退到 TCP 长连接...", false);
    _chat_endpoint_state.connectStartedMs = QDateTime::currentMSecsSinceEpoch();
    _gateway.chatTransport()->connectToServer(serverInfo);
    return true;
}

bool AppController::tryReconnectChat()
{
    if (_page != ChatPage)
    {
        return false;
    }
    if (_chat_endpoint_state.endpoints.isEmpty())
    {
        return false;
    }
    const bool missingCredential = _chat_endpoint_state.protocolVersion >= 3
                                       ? _pending_login_state.loginTicket.isEmpty()
                                       : _pending_login_state.token.isEmpty();
    if (_pending_login_state.uid <= 0 || missingCredential)
    {
        return false;
    }
    if (_chat_recovery_state.reconnectAttempts >= kChatReconnectMaxAttempts)
    {
        qWarning() << "Reconnect attempts exhausted for uid:" << _pending_login_state.uid
                   << "host:" << _chat_endpoint_state.host << "port:" << _chat_endpoint_state.port;
        return false;
    }

    _chat_recovery_state.reconnecting = true;
    ++_chat_recovery_state.reconnectAttempts;
    qInfo() << "Attempting chat reconnect" << _chat_recovery_state.reconnectAttempts << "/" << kChatReconnectMaxAttempts
            << "uid:" << _pending_login_state.uid << "host:" << _chat_endpoint_state.host
            << "port:" << _chat_endpoint_state.port;
    setTip(QString("聊天连接断开，正在重连（%1/%2）...")
               .arg(_chat_recovery_state.reconnectAttempts)
               .arg(kChatReconnectMaxAttempts),
           true);

    ServerInfo serverInfo;
    serverInfo.Uid = _pending_login_state.uid;
    serverInfo.Token = _pending_login_state.token;
    serverInfo.LoginTicket = _pending_login_state.loginTicket;
    serverInfo.ProtocolVersion = _chat_endpoint_state.protocolVersion;
    serverInfo.Endpoints = _chat_endpoint_state.endpoints;
    serverInfo.PreferredTransport = hasEndpointKind(_chat_endpoint_state.endpoints, ChatTransportKind::Quic)
                                        ? ChatTransportKind::Quic
                                        : ChatTransportKind::Tcp;
    serverInfo.FallbackTransport = ChatTransportKind::Tcp;
    serverInfo.ConnectTimeoutMs = _chat_endpoint_state.connectTimeoutMs;
    serverInfo.BackupDialDelayMs = _chat_endpoint_state.backupDialDelayMs;
    serverInfo.TotalLoginTimeoutMs = _chat_endpoint_state.totalLoginTimeoutMs;
    _chat_endpoint_state.endpointIndex = 0;
    _chat_endpoint_state.host = _chat_endpoint_state.endpoints.front().host;
    _chat_endpoint_state.port = _chat_endpoint_state.endpoints.front().port;
    _chat_endpoint_state.serverName = _chat_endpoint_state.endpoints.front().serverName;
    serverInfo.Host = _chat_endpoint_state.host;
    serverInfo.Port = _chat_endpoint_state.port;
    serverInfo.ServerName = _chat_endpoint_state.serverName;
    QTimer::singleShot(kChatReconnectDelayMs,
                       this,
                       [this, serverInfo]()
                       {
                           if (!_chat_recovery_state.reconnecting || _page != ChatPage)
                           {
                               return;
                           }
                           _chat_endpoint_state.connectStartedMs = QDateTime::currentMSecsSinceEpoch();
                           _gateway.chatTransport()->connectToServer(serverInfo);
                       });
    return true;
}

void AppController::handleChatConnectFailureDuringRecovery()
{
    if (tryReconnectChat())
    {
        return;
    }
    _heartbeat_timer.stop();
    switchToLogin();
    setTip("聊天重连失败，请重新登录", true);
}

void AppController::resetReconnectState()
{
    _chat_recovery_state.reconnecting = false;
    _chat_recovery_state.reconnectAttempts = 0;
}

void AppController::resetHeartbeatTracking()
{
    _chat_recovery_state.lastHeartbeatSentMs = 0;
    _chat_recovery_state.lastHeartbeatAckMs = 0;
    _chat_recovery_state.heartbeatAckMissCount = 0;
    _chat_recovery_state.closedByHeartbeatWatchdog = false;
}

bool AppController::isHeartbeatLikelyTimeout() const
{
    if (_chat_recovery_state.closedByHeartbeatWatchdog)
    {
        return true;
    }
    if (_chat_recovery_state.lastHeartbeatSentMs <= 0)
    {
        return false;
    }
    if (_chat_recovery_state.lastHeartbeatAckMs >= _chat_recovery_state.lastHeartbeatSentMs)
    {
        return false;
    }
    return (QDateTime::currentMSecsSinceEpoch() - _chat_recovery_state.lastHeartbeatSentMs) >= kHeartbeatAckGraceMs;
}

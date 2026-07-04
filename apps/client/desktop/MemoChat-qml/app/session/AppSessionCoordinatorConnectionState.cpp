#include "AppCoordinators.h"

#include <QString>

const AppPendingLoginState& AppSessionCoordinator::pendingLoginState() const
{
    return _pending_login_state;
}

const AppChatEndpointState& AppSessionCoordinator::chatEndpointState() const
{
    return _chat_endpoint_state;
}

const AppChatRecoveryState& AppSessionCoordinator::chatRecoveryState() const
{
    return _chat_recovery_state;
}

QString AppSessionCoordinator::authToken() const
{
    return _pending_login_state.token;
}

int AppSessionCoordinator::pendingUid() const
{
    return _pending_login_state.uid;
}

void AppSessionCoordinator::resetLoginAttemptState(qint64 loginStartedMs)
{
    _pending_login_state.uid = 0;
    _pending_login_state.token.clear();
    _pending_login_state.loginTicket.clear();
    _pending_login_state.refreshToken.clear();
    _pending_login_state.traceId.clear();
    _chat_endpoint_state.endpoints.clear();
    _chat_endpoint_state.endpointIndex = -1;
    _chat_endpoint_state.serverName.clear();
    _chat_endpoint_state.loginStartedMs = loginStartedMs;
    _chat_endpoint_state.httpLoginFinishedMs = 0;
    _chat_endpoint_state.connectStartedMs = 0;
    _chat_endpoint_state.connectFinishedMs = 0;
    _chat_recovery_state.ignoreNextLoginDisconnect = true;
}

void AppSessionCoordinator::applyLoginSuccessState(const ServerInfo& serverInfo,
                                                   const QString& traceId,
                                                   qint64 httpLoginFinishedMs)
{
    _pending_login_state.uid = serverInfo.Uid;
    _pending_login_state.token = serverInfo.Token;
    _pending_login_state.loginTicket = serverInfo.LoginTicket;
    _pending_login_state.refreshToken = serverInfo.RefreshToken;
    _pending_login_state.traceId = traceId;

    _chat_endpoint_state.protocolVersion = serverInfo.ProtocolVersion;
    _chat_endpoint_state.endpoints = serverInfo.Endpoints;
    _chat_endpoint_state.endpointIndex = 0;
    _chat_endpoint_state.connectTimeoutMs = serverInfo.ConnectTimeoutMs;
    _chat_endpoint_state.backupDialDelayMs = serverInfo.BackupDialDelayMs;
    _chat_endpoint_state.totalLoginTimeoutMs = serverInfo.TotalLoginTimeoutMs;
    _chat_endpoint_state.httpLoginFinishedMs = httpLoginFinishedMs;
    if (!_chat_endpoint_state.endpoints.isEmpty())
    {
        const ChatEndpoint& endpoint = _chat_endpoint_state.endpoints.front();
        _chat_endpoint_state.host = endpoint.host;
        _chat_endpoint_state.port = endpoint.port;
        _chat_endpoint_state.serverName = endpoint.serverName;
    }
    else
    {
        _chat_endpoint_state.host.clear();
        _chat_endpoint_state.port.clear();
        _chat_endpoint_state.serverName.clear();
    }

    _chat_recovery_state.loginTcpFallbackAttempted = false;
}

void AppSessionCoordinator::clearSessionForLogout()
{
    _pending_login_state.uid = 0;
    _pending_login_state.token.clear();
    _pending_login_state.loginTicket.clear();
    _pending_login_state.refreshToken.clear();
    _pending_login_state.traceId.clear();

    _chat_endpoint_state.host.clear();
    _chat_endpoint_state.port.clear();
    _chat_endpoint_state.serverName.clear();
    _chat_endpoint_state.endpoints.clear();
    _chat_endpoint_state.endpointIndex = -1;

    _chat_recovery_state.ignoreNextLoginDisconnect = true;
    _chat_recovery_state.loginTcpFallbackAttempted = false;
}

void AppSessionCoordinator::setIgnoreNextLoginDisconnect(bool ignore)
{
    _chat_recovery_state.ignoreNextLoginDisconnect = ignore;
}

void AppSessionCoordinator::setReconnecting(bool reconnecting)
{
    _chat_recovery_state.reconnecting = reconnecting;
}

void AppSessionCoordinator::setReconnectAttempts(int attempts)
{
    _chat_recovery_state.reconnectAttempts = attempts;
}

void AppSessionCoordinator::setLoginTcpFallbackAttempted(bool attempted)
{
    _chat_recovery_state.loginTcpFallbackAttempted = attempted;
}

void AppSessionCoordinator::setConnectStartedMs(qint64 connectStartedMs)
{
    _chat_endpoint_state.connectStartedMs = connectStartedMs;
}

void AppSessionCoordinator::setConnectFinishedMs(qint64 connectFinishedMs)
{
    _chat_endpoint_state.connectFinishedMs = connectFinishedMs;
}

void AppSessionCoordinator::setCurrentEndpoint(const ChatEndpoint& endpoint)
{
    _chat_endpoint_state.host = endpoint.host;
    _chat_endpoint_state.port = endpoint.port;
    _chat_endpoint_state.serverName = endpoint.serverName;
}

void AppSessionCoordinator::setEndpointIndex(int endpointIndex)
{
    _chat_endpoint_state.endpointIndex = endpointIndex;
}

void AppSessionCoordinator::setLastHeartbeatSentMs(qint64 sentMs)
{
    _chat_recovery_state.lastHeartbeatSentMs = sentMs;
}

void AppSessionCoordinator::setLastHeartbeatAckMs(qint64 ackMs)
{
    _chat_recovery_state.lastHeartbeatAckMs = ackMs;
}

void AppSessionCoordinator::setHeartbeatAckMissCount(int missCount)
{
    _chat_recovery_state.heartbeatAckMissCount = missCount;
}

void AppSessionCoordinator::setClosedByHeartbeatWatchdog(bool closed)
{
    _chat_recovery_state.closedByHeartbeatWatchdog = closed;
}

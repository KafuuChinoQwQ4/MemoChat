#include "AppChatConnectionCoordinator.h"
#include "AppCoordinators.h"
#include "AppPortRegistry.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QTimer>
#include <utility>

void AppPortRegistry::bindConnectionPorts()
{
    _chat_connection_coordinator = std::make_unique<AppChatConnectionCoordinator>(
        ChatConnectionPort{[this]()
                           {
                               ChatConnectionSnapshot snapshot;
                               const AppPendingLoginState& pending = _session_coordinator->pendingLoginState();
                               const AppChatEndpointState& endpointState = _session_coordinator->chatEndpointState();
                               const AppChatRecoveryState& recovery = _session_coordinator->chatRecoveryState();
                               snapshot.page = _shell_state.page();
                               snapshot.isLoginPage = page() == _constants.loginPage;
                               snapshot.isChatPage = page() == _constants.chatPage;
                               snapshot.busy = busy();
                               snapshot.uid = pending.uid;
                               snapshot.token = pending.token;
                               snapshot.loginTicket = pending.loginTicket;
                               snapshot.traceId = pending.traceId;
                               snapshot.host = endpointState.host;
                               snapshot.port = endpointState.port;
                               snapshot.serverName = endpointState.serverName;
                               snapshot.endpoints = endpointState.endpoints;
                               snapshot.endpointIndex = endpointState.endpointIndex;
                               snapshot.connectTimeoutMs = endpointState.connectTimeoutMs;
                               snapshot.backupDialDelayMs = endpointState.backupDialDelayMs;
                               snapshot.totalLoginTimeoutMs = endpointState.totalLoginTimeoutMs;
                               snapshot.protocolVersion = endpointState.protocolVersion;
                               snapshot.connectStartedMs = endpointState.connectStartedMs;
                               snapshot.connectFinishedMs = endpointState.connectFinishedMs;
                               snapshot.lastHeartbeatSentMs = recovery.lastHeartbeatSentMs;
                               snapshot.lastHeartbeatAckMs = recovery.lastHeartbeatAckMs;
                               snapshot.heartbeatAckMissCount = recovery.heartbeatAckMissCount;
                               snapshot.closedByHeartbeatWatchdog = recovery.closedByHeartbeatWatchdog;
                               snapshot.reconnecting = recovery.reconnecting;
                               snapshot.reconnectAttempts = recovery.reconnectAttempts;
                               snapshot.ignoreNextLoginDisconnect = recovery.ignoreNextLoginDisconnect;
                               snapshot.loginTcpFallbackAttempted = recovery.loginTcpFallbackAttempted;
                               return snapshot;
                           },
                           [this](const QString& tip, bool isError)
                           {
                               setTip(tip, isError);
                           },
                           [this](bool value)
                           {
                               setBusy(value);
                           },
                           [this]()
                           {
                               switchToLogin();
                           },
                           [this]()
                           {
                               return _chat_login_timeout_timer.isActive();
                           },
                           [this]()
                           {
                               _chat_login_timeout_timer.start();
                           },
                           [this]()
                           {
                               _chat_login_timeout_timer.stop();
                           },
                           [this]()
                           {
                               _heartbeat_timer.stop();
                           },
                           [this](bool ignore)
                           {
                               _session_coordinator->setIgnoreNextLoginDisconnect(ignore);
                           },
                           [this](bool reconnecting)
                           {
                               _session_coordinator->setReconnecting(reconnecting);
                           },
                           [this](int attempts)
                           {
                               _session_coordinator->setReconnectAttempts(attempts);
                           },
                           [this](bool attempted)
                           {
                               _session_coordinator->setLoginTcpFallbackAttempted(attempted);
                           },
                           [this](qint64 connectStartedMs)
                           {
                               _session_coordinator->setConnectStartedMs(connectStartedMs);
                           },
                           [this](qint64 connectFinishedMs)
                           {
                               _session_coordinator->setConnectFinishedMs(connectFinishedMs);
                           },
                           [this](const ChatEndpoint& endpoint)
                           {
                               _session_coordinator->setCurrentEndpoint(endpoint);
                           },
                           [this](int endpointIndex)
                           {
                               _session_coordinator->setEndpointIndex(endpointIndex);
                           },
                           [this](qint64 sentMs)
                           {
                               _session_coordinator->setLastHeartbeatSentMs(sentMs);
                           },
                           [this](qint64 ackMs)
                           {
                               _session_coordinator->setLastHeartbeatAckMs(ackMs);
                           },
                           [this](int missCount)
                           {
                               _session_coordinator->setHeartbeatAckMissCount(missCount);
                           },
                           [this](bool closed)
                           {
                               _session_coordinator->setClosedByHeartbeatWatchdog(closed);
                           },
                           [this]()
                           {
                               return _gateway.userMgr() ? _gateway.userMgr()->GetUid() : 0;
                           },
                           [this]()
                           {
                               return _features.callController.callVisible();
                           },
                           [this](const QString& statusText)
                           {
                               if (_call_coordinator)
                               {
                                   _call_coordinator->finalizeEndedCall(statusText);
                               }
                           },
                           [this](const QByteArray& payload)
                           {
                               _gateway.chatTransport()->slot_send_data(ReqId::ID_CHAT_LOGIN, payload);
                           },
                           [this](const QByteArray& payload)
                           {
                               _gateway.chatTransport()->slot_send_data(ReqId::ID_HEART_BEAT_REQ, payload);
                           },
                           [this](const ServerInfo& serverInfo)
                           {
                               _gateway.chatTransport()->connectToServer(serverInfo);
                           },
                           [this]()
                           {
                               _gateway.chatTransport()->CloseConnection();
                           },
                           [this](int delayMs, std::function<void()> callback)
                           {
                               QTimer::singleShot(delayMs, &_async_receiver, std::move(callback));
                           }});
}

#pragma once

#include "global.h"

#include <QByteArray>
#include <QVector>
#include <QtGlobal>
#include <QString>
#include <functional>

struct ChatConnectionSnapshot
{
    int page = 0;
    bool isLoginPage = false;
    bool isChatPage = false;
    bool busy = false;

    int uid = 0;
    QString token;
    QString loginTicket;
    QString traceId;

    QString host;
    QString port;
    QString serverName;
    QVector<ChatEndpoint> endpoints;
    int endpointIndex = -1;
    int connectTimeoutMs = 1200;
    int backupDialDelayMs = 250;
    int totalLoginTimeoutMs = 5000;
    int protocolVersion = 2;
    qint64 connectStartedMs = 0;
    qint64 connectFinishedMs = 0;

    qint64 lastHeartbeatSentMs = 0;
    qint64 lastHeartbeatAckMs = 0;
    int heartbeatAckMissCount = 0;
    bool closedByHeartbeatWatchdog = false;
    bool reconnecting = false;
    int reconnectAttempts = 0;
    bool ignoreNextLoginDisconnect = false;
    bool loginTcpFallbackAttempted = false;
};

struct ChatConnectionPort
{
    std::function<ChatConnectionSnapshot()> snapshot;
    std::function<void(const QString&, bool)> setTip;
    std::function<void(bool)> setBusy;
    std::function<void()> switchToLogin;
    std::function<bool()> loginTimeoutTimerActive;
    std::function<void()> startLoginTimeoutTimer;
    std::function<void()> stopLoginTimeoutTimer;
    std::function<void()> stopHeartbeatTimer;
    std::function<void(bool)> setIgnoreNextLoginDisconnect;
    std::function<void(bool)> setReconnecting;
    std::function<void(int)> setReconnectAttempts;
    std::function<void(bool)> setLoginTcpFallbackAttempted;
    std::function<void(qint64)> setConnectStartedMs;
    std::function<void(qint64)> setConnectFinishedMs;
    std::function<void(const ChatEndpoint&)> setCurrentEndpoint;
    std::function<void(int)> setEndpointIndex;
    std::function<void(qint64)> setLastHeartbeatSentMs;
    std::function<void(qint64)> setLastHeartbeatAckMs;
    std::function<void(int)> setHeartbeatAckMissCount;
    std::function<void(bool)> setClosedByHeartbeatWatchdog;
    std::function<int()> currentUserUid;
    std::function<bool()> callVisible;
    std::function<void(const QString&)> finalizeEndedCall;
    std::function<void(const QByteArray&)> sendChatLogin;
    std::function<void(const QByteArray&)> sendHeartbeat;
    std::function<void(const ServerInfo&)> connectToServer;
    std::function<void()> closeConnection;
    std::function<void(int, std::function<void()>)> runDelayedReconnect;
};

class AppChatConnectionCoordinator
{
public:
    explicit AppChatConnectionCoordinator(ChatConnectionPort port);

    void onTcpConnectFinished(bool success);
    void onChatLoginFailed(int err);
    void onConnectionClosed();
    void onLoginTimeout();
    void onHeartbeatTimeout();
    void onHeartbeatAck(qint64 ackAtMs);
    void onNotifyOffline();

    bool tryLoginFallbackToTcp(const QString& reason);
    bool tryReconnectChat();
    void handleChatConnectFailureDuringRecovery();
    void resetReconnectState();
    void resetHeartbeatTracking();
    bool isHeartbeatLikelyTimeout() const;

private:
    ChatConnectionPort _port;
};

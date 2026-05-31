#pragma once

#include <QtGlobal>
#include <QString>

class AppController;

class AppChatConnectionCoordinator
{
public:
    explicit AppChatConnectionCoordinator(AppController& controller);

    void onTcpConnectFinished(bool success);
    void onChatLoginFailed(int err);
    void onConnectionClosed();
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
    AppController& _app;
};

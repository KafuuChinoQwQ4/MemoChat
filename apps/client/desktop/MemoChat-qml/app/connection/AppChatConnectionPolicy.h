#pragma once

#include "global.h"

#include <QString>
#include <QVector>

namespace AppChatConnectionPolicy
{
inline constexpr qint64 kHeartbeatAckGraceMs = 15000;
inline constexpr int kChatReconnectMaxAttempts = 2;
inline constexpr int kChatReconnectDelayMs = 300;

struct AppChatConnectionSnapshot
{
    bool isLoginPage = false;
    bool isChatPage = false;
    bool busy = false;
    bool loginTcpFallbackAttempted = false;
    int reconnectAttempts = 0;
    int uid = 0;
    QString token;
    QString loginTicket;
    QVector<ChatEndpoint> endpoints;
    int connectTimeoutMs = 1200;
    int backupDialDelayMs = 250;
    int totalLoginTimeoutMs = 5000;
    int protocolVersion = 2;
};

struct AppChatConnectionDecision
{
    bool allowed = false;
    ServerInfo serverInfo;
    ChatEndpoint selectedEndpoint;
};

bool hasEndpointKind(const QVector<ChatEndpoint>& endpoints, ChatTransportKind kind);
AppChatConnectionDecision evaluateLoginTcpFallback(const AppChatConnectionSnapshot& snapshot);
AppChatConnectionDecision evaluateReconnect(const AppChatConnectionSnapshot& snapshot);
} // namespace AppChatConnectionPolicy

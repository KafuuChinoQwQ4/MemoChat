#pragma once

#include "global.h"

#include <QString>
#include <QVector>
#include <QtGlobal>

struct AppPendingLoginState
{
    int uid = 0;
    QString token;
    QString loginTicket;
    QString refreshToken;
    QString traceId;
};

struct AppChatEndpointState
{
    QString host;
    QString port;
    QString serverName;
    QVector<ChatEndpoint> endpoints;
    int endpointIndex = -1;
    int connectTimeoutMs = 1200;
    int backupDialDelayMs = 250;
    int totalLoginTimeoutMs = 5000;
    int protocolVersion = 3;
    qint64 loginStartedMs = 0;
    qint64 httpLoginFinishedMs = 0;
    qint64 connectStartedMs = 0;
    qint64 connectFinishedMs = 0;
};

struct AppChatRecoveryState
{
    qint64 lastHeartbeatSentMs = 0;
    qint64 lastHeartbeatAckMs = 0;
    int heartbeatAckMissCount = 0;
    bool closedByHeartbeatWatchdog = false;
    bool reconnecting = false;
    int reconnectAttempts = 0;
    bool ignoreNextLoginDisconnect = false;
    bool loginTcpFallbackAttempted = false;
};

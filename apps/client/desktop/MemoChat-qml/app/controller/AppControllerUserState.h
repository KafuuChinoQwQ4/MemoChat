#pragma once

#include <QString>

struct AppCurrentUserState
{
    QString name;
    QString nick;
    QString icon = QStringLiteral("qrc:/res/head_1.png");
    QString desc;
    QString userId;
};

struct AppCurrentContactState
{
    int uid = 0;
    QString name;
    QString nick;
    QString icon = QStringLiteral("qrc:/res/head_1.png");
    QString back;
    int sex = 0;
    QString userId;
};

struct AppCurrentChatState
{
    QString peerName;
    QString peerIcon = QStringLiteral("qrc:/res/head_1.png");
    int uid = 0;
};

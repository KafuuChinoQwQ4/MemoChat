#ifndef CHATREADACKCOMMAND_H
#define CHATREADACKCOMMAND_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>
#include <functional>

struct ChatReadAckDispatchPort
{
    std::function<void(int, const QByteArray&)> dispatchPayload;
};

struct ChatReadAckPort
{
    std::function<int()> selfUid;
    ChatReadAckDispatchPort dispatch;
};

struct ChatReadAckCommand
{
    int selfUid = 0;
    int peerUid = 0;
    qint64 groupId = 0;
    qint64 readTs = 0;
};

struct ChatReadAckCommandResult
{
    bool success = false;
    bool skipped = false;
    bool dispatched = false;
    bool privatePath = false;
    bool groupPath = false;
    int requestId = 0;
    int selfUid = 0;
    int peerUid = 0;
    qint64 groupId = 0;
    qint64 readTs = 0;
    QByteArray compactPayload;
    QString errorText;
};

#endif // CHATREADACKCOMMAND_H

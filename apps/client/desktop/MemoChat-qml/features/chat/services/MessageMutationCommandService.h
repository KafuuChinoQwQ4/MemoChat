#ifndef MESSAGEMUTATIONCOMMANDSERVICE_H
#define MESSAGEMUTATIONCOMMANDSERVICE_H

#include <QByteArray>
#include <QString>
#include <QtGlobal>
#include <functional>

enum class MessageMutationCommand
{
    Edit,
    Revoke,
    Forward
};

struct MessageMutationTarget
{
    qint64 groupId = 0;
    int peerUid = 0;

    bool isGroup() const
    {
        return groupId > 0;
    }

    bool isPrivate() const
    {
        return groupId <= 0 && peerUid > 0;
    }
};

struct MessageMutationCommandRequest
{
    int selfUid = 0;
    MessageMutationCommand command = MessageMutationCommand::Edit;
    MessageMutationTarget target;
    QString msgId;
    QString text;
};

struct MessageMutationCommandDependencies
{
    std::function<bool(const QString&)> privateMessageExists;
    std::function<void(const QString&, bool)> setPrivateStatus;
    std::function<void(const QString&, bool)> setGroupStatus;
    std::function<void(int, const QByteArray&)> dispatchPayload;
};

struct MessageMutationCommandResult
{
    bool success = false;
    QString errorText;
    bool errorIsGroup = false;
    int reqId = 0;
    QByteArray compactPayload;
    bool dispatched = false;
};

class MessageMutationCommandService
{
public:
    static MessageMutationCommandResult run(const MessageMutationCommandRequest& request,
                                            const MessageMutationCommandDependencies& dependencies);
    static QByteArray buildPayload(const MessageMutationCommandRequest& request);
    static int requestIdFor(MessageMutationCommand command, const MessageMutationTarget& target);
};

#endif // MESSAGEMUTATIONCOMMANDSERVICE_H

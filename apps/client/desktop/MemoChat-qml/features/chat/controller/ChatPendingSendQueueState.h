#ifndef CHATPENDINGSENDQUEUESTATE_H
#define CHATPENDINGSENDQUEUESTATE_H

#include <QVariantList>
#include <QVariantMap>
#include <QtGlobal>

struct ChatPendingAttachmentCommandResult
{
    int dialogUid = 0;
    bool valid = false;
    bool changed = false;
    QVariantList attachments;
};

struct ChatPendingSendQueueSnapshot
{
    bool hasCurrent = false;
    QVariantMap currentAttachment;
    int dialogUid = 0;
    int chatUid = 0;
    qint64 groupId = 0;
    int totalCount = 0;
    int remainingCount = 0;
    int currentIndex = 0;
};

struct ChatPendingSendQueueState
{
    QVariantList queue;
    int dialogUid = 0;
    int chatUid = 0;
    qint64 groupId = 0;
    int totalCount = 0;

    void reset()
    {
        queue.clear();
        dialogUid = 0;
        chatUid = 0;
        groupId = 0;
        totalCount = 0;
    }

    ChatPendingSendQueueSnapshot snapshot() const
    {
        ChatPendingSendQueueSnapshot result;
        result.hasCurrent = !queue.isEmpty();
        result.dialogUid = dialogUid;
        result.chatUid = chatUid;
        result.groupId = groupId;
        result.totalCount = totalCount;
        result.remainingCount = queue.size();
        result.currentIndex = result.hasCurrent ? qMax(1, totalCount - queue.size() + 1) : 0;
        if (result.hasCurrent)
        {
            result.currentAttachment = queue.first().toMap();
        }
        return result;
    }
};

#endif // CHATPENDINGSENDQUEUESTATE_H

#include "ConversationSyncService.h"

#include "ChatMessageModel.h"
#include "FriendListModel.h"

#include <limits>

void ConversationSyncService::syncHistoryCursor(const ChatMessageModel &messageModel,
                                                qint64 &beforeTs,
                                                QString &beforeMsgId)
{
    beforeTs = messageModel.earliestCreatedAt();
    beforeMsgId = messageModel.earliestMsgId();
}

void ConversationSyncService::updatePrivatePreview(FriendListModel &chatListModel,
                                                   FriendListModel &dialogListModel,
                                                   int peerUid,
                                                   const QString &preview,
                                                   qint64 lastMsgTs)
{
    if (peerUid <= 0) {
        return;
    }
    chatListModel.updateLastMessage(peerUid, preview, lastMsgTs);
    dialogListModel.updateLastMessage(peerUid, preview, lastMsgTs);
}

void ConversationSyncService::clearPrivateUnread(FriendListModel &chatListModel,
                                                 FriendListModel &dialogListModel,
                                                 int peerUid)
{
    if (peerUid <= 0) {
        return;
    }
    dialogListModel.clearUnread(peerUid);
    chatListModel.clearUnread(peerUid);
}

void ConversationSyncService::incrementPrivateUnread(FriendListModel &dialogListModel, int peerUid)
{
    if (peerUid <= 0) {
        return;
    }
    dialogListModel.incrementUnread(peerUid);
}

int ConversationSyncService::makeGroupDialogUid(qint64 groupId)
{
    if (groupId <= 0 || groupId > static_cast<qint64>(std::numeric_limits<int>::max())) {
        return 0;
    }
    return -static_cast<int>(groupId);
}

qint64 ConversationSyncService::groupIdForDialogUid(const QMap<int, qint64> &groupUidMap, int dialogUid)
{
    if (dialogUid >= 0) {
        return 0;
    }
    const qint64 mappedGroupId = groupUidMap.value(dialogUid, 0);
    if (mappedGroupId > 0) {
        return mappedGroupId;
    }
    return -static_cast<qint64>(dialogUid);
}

int ConversationSyncService::resolveGroupDialogUid(QMap<int, qint64> &groupUidMap, qint64 groupId)
{
    if (groupId <= 0) {
        return 0;
    }
    for (auto it = groupUidMap.cbegin(); it != groupUidMap.cend(); ++it) {
        if (it.value() == groupId) {
            return it.key();
        }
    }
    const int dialogUid = makeGroupDialogUid(groupId);
    if (dialogUid == 0) {
        return 0;
    }
    groupUidMap.insert(dialogUid, groupId);
    return dialogUid;
}

void ConversationSyncService::updateGroupPreview(FriendListModel &groupListModel,
                                                 FriendListModel &dialogListModel,
                                                 int dialogUid,
                                                 const QString &groupPreview,
                                                 const QString &dialogPreview,
                                                 qint64 lastMsgTs)
{
    if (dialogUid >= 0) {
        return;
    }
    groupListModel.updateLastMessage(dialogUid, groupPreview, lastMsgTs);
    dialogListModel.updateLastMessage(dialogUid, dialogPreview, lastMsgTs);
}

void ConversationSyncService::clearGroupUnreadAndMention(FriendListModel &dialogListModel,
                                                         QHash<int, int> &mentionMap,
                                                         int dialogUid)
{
    if (dialogUid >= 0) {
        return;
    }
    dialogListModel.clearUnread(dialogUid);
    dialogListModel.clearMention(dialogUid);
    mentionMap.remove(dialogUid);
}

void ConversationSyncService::incrementGroupUnreadAndMention(FriendListModel &dialogListModel,
                                                             QHash<int, int> &mentionMap,
                                                             int dialogUid,
                                                             bool mentionedMe)
{
    if (dialogUid >= 0) {
        return;
    }
    dialogListModel.incrementUnread(dialogUid);
    if (!mentionedMe) {
        return;
    }
    const int nextMentionCount = mentionMap.value(dialogUid, 0) + 1;
    mentionMap.insert(dialogUid, nextMentionCount);
    dialogListModel.setMentionCount(dialogUid, nextMentionCount);
}

#ifndef CONVERSATIONSYNCSERVICE_H
#define CONVERSATIONSYNCSERVICE_H

#include <QHash>
#include <QMap>
#include <QString>

class ChatMessageModel;
class FriendListModel;

class ConversationSyncService
{
public:
    static void syncHistoryCursor(const ChatMessageModel &messageModel,
                                  qint64 &beforeTs,
                                  QString &beforeMsgId);

    static void updatePrivatePreview(FriendListModel &chatListModel,
                                     FriendListModel &dialogListModel,
                                     int peerUid,
                                     const QString &preview,
                                     qint64 lastMsgTs);
    static void clearPrivateUnread(FriendListModel &chatListModel,
                                   FriendListModel &dialogListModel,
                                   int peerUid);
    static void incrementPrivateUnread(FriendListModel &dialogListModel, int peerUid);

    static int makeGroupDialogUid(qint64 groupId);
    static qint64 groupIdForDialogUid(const QMap<int, qint64> &groupUidMap, int dialogUid);
    static int resolveGroupDialogUid(QMap<int, qint64> &groupUidMap, qint64 groupId);
    static void updateGroupPreview(FriendListModel &groupListModel,
                                   FriendListModel &dialogListModel,
                                   int dialogUid,
                                   const QString &groupPreview,
                                   const QString &dialogPreview,
                                   qint64 lastMsgTs);
    static void clearGroupUnreadAndMention(FriendListModel &dialogListModel,
                                           QHash<int, int> &mentionMap,
                                           int dialogUid);
    static void incrementGroupUnreadAndMention(FriendListModel &dialogListModel,
                                               QHash<int, int> &mentionMap,
                                               int dialogUid,
                                               bool mentionedMe);
};

#endif // CONVERSATIONSYNCSERVICE_H

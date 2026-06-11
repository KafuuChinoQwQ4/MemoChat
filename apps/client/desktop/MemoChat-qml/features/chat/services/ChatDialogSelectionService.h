#ifndef CHATDIALOGSELECTIONSERVICE_H
#define CHATDIALOGSELECTIONSERVICE_H

#include <QMap>
#include <QString>
#include <QVariantMap>
#include <functional>
#include <memory>

#include "DialogListTypes.h"

class FriendListModel;
struct AuthInfo;
struct FriendInfo;
struct GroupInfoData;

struct ChatGroupDialogLookupPort
{
    std::function<QVariantMap(int)> groupDialogByIndex;
    std::function<int(int)> indexOfGroupDialog;
    std::function<QVariantMap(int)> groupDialogItem;
    std::function<qint64(int)> groupIdForDialogUid;
    std::function<void(int)> clearGroupUnread;
};

struct ChatDialogSelectionPort
{
    FriendListModel* chatListModel = nullptr;
    FriendListModel* dialogListModel = nullptr;
    ChatGroupDialogLookupPort groupDialogs;
    std::function<void()> ensureGroupsInitialized;
    std::function<std::shared_ptr<FriendInfo>(int)> friendById;
    std::function<void(std::shared_ptr<AuthInfo>)> addFriend;
    std::function<void(std::shared_ptr<AuthInfo>)> upsertContact;
    std::function<std::shared_ptr<GroupInfoData>(qint64)> groupById;
    std::function<void(std::shared_ptr<GroupInfoData>)> upsertGroup;
    std::function<DialogDecorationState()> dialogDecorationState;
    std::function<void(const QString&, const QString&, const QString&)> setPendingReplyContext;
    std::function<void(int)> setCurrentPrivatePeerUid;
    std::function<void(qint64, const QString&, const QString&)> setCurrentGroup;
    std::function<void(const QString&)> setCurrentChatPeerName;
    std::function<void(const QString&)> setCurrentChatPeerIcon;
    std::function<void()> clearMessageModel;
    std::function<void()> clearPrivateHistoryState;
    std::function<void()> resetGroupConversationState;
    std::function<void(bool)> setGroupHistoryLoading;
    std::function<void(bool)> setPrivateHistoryLoading;
    std::function<void(bool)> setCanLoadMorePrivateHistory;
    std::function<void(int)> removeMentionForDialog;
    std::function<void()> emitCurrentDialogUidChangedIfNeeded;
    std::function<void()> loadCurrentPrivateMessages;
    std::function<void()> syncCurrentDialogDraft;
    std::function<qint64(qint64)> openGroupConversation;
    std::function<void(qint64, qint64)> sendGroupReadAck;
    std::function<void()> loadGroupHistory;
};

class ChatDialogSelectionService
{
public:
    static void selectPrivateByIndex(int index, const ChatDialogSelectionPort& port);
    static void selectPrivateByUid(int uid, const ChatDialogSelectionPort& port);
    static void selectGroupByIndex(int index, const ChatDialogSelectionPort& port);
    static void selectGroupByDialogUid(int dialogUid, qint64 groupId, const ChatDialogSelectionPort& port);
    static void selectDialogByUid(int dialogUid, const ChatDialogSelectionPort& port);
};

#endif // CHATDIALOGSELECTIONSERVICE_H

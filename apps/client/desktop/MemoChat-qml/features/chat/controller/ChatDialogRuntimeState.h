#ifndef CHATDIALOGRUNTIMESTATE_H
#define CHATDIALOGRUNTIMESTATE_H

#include "ChatReadAckCommand.h"

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QSet>
#include <QString>
#include <QVariantList>
#include <functional>
#include <memory>

class FriendListModel;
struct FriendInfo;
struct GroupInfoData;

struct ChatDialogRuntimeSnapshot
{
    int dialogUid = 0;
    bool hasDialog = false;
    QString draftText;
    QVariantList pendingAttachments;
    bool pinned = false;
    bool muted = false;
};

struct ChatDialogDraftResult
{
    int dialogUid = 0;
    bool valid = false;
    QString draftText;
};

struct ChatDialogPinnedResult
{
    int dialogUid = 0;
    bool valid = false;
    bool pinned = false;
    int pinnedRank = 0;
};

struct ChatDialogMutedResult
{
    int dialogUid = 0;
    bool valid = false;
    bool muted = false;
    int muteState = 0;
    QString draftText;
};

struct ChatDialogTarget
{
    QString dialogType;
    int peerUid = 0;
    qint64 groupId = 0;

    bool isPrivate() const
    {
        return dialogType == QStringLiteral("private") && peerUid > 0;
    }

    bool isGroup() const
    {
        return dialogType == QStringLiteral("group") && groupId > 0;
    }
};

struct ChatMarkDialogReadRequest
{
    int selfUid = 0;
    int dialogUid = 0;
    ChatDialogTarget target;
    qint64 latestPrivatePeerTs = 0;
    qint64 latestGroupTs = 0;
};

struct ChatDialogCommandSnapshot
{
    int selfUid = 0;
    int ownerUid = 0;
    int currentDialogUid = 0;
    int currentPrivatePeerUid = 0;
    qint64 currentGroupId = 0;
};

struct ChatMarkDialogReadDependencies
{
    FriendListModel* dialogListModel = nullptr;
    FriendListModel* privateChatListModel = nullptr;
    FriendListModel* groupListModel = nullptr;
    ChatReadAckDispatchPort readAckDispatchPort;
};

struct ChatMarkDialogReadResult
{
    bool success = false;
    int dialogUid = 0;
    bool dialogUnreadCleared = false;
    bool mentionCleared = false;
    bool privateUnreadCleared = false;
    bool groupUnreadCleared = false;
    bool privateReadAckDispatched = false;
    bool groupReadAckDispatched = false;
    qint64 readAckTs = 0;
};

struct ChatDialogCommandRequest
{
    int selfUid = 0;
    int ownerUid = 0;
    int currentDialogUid = 0;
    int dialogUid = 0;
    ChatDialogTarget target;
    QString draftText;
};

struct ChatDialogCommandDependencies
{
    FriendListModel* dialogListModel = nullptr;
    std::function<void(int, const QByteArray&)> dispatchPayload;
    std::function<void(int)> savePersistentDialogStore;
    std::function<void()> refreshDialogModel;
};

struct ChatDialogCommandResult
{
    bool success = false;
    int dialogUid = 0;
    QByteArray compactPayload;
    bool currentDraftUpdated = false;
    bool currentPinnedUpdated = false;
    bool currentMutedUpdated = false;
    bool dialogModelUpdated = false;
    bool persistentStoreSaved = false;
    bool dispatched = false;
    bool dialogModelRefreshed = false;
};

struct ChatDialogCommandPort
{
    std::function<ChatDialogCommandSnapshot()> snapshot;
    std::function<ChatDialogTarget(int)> targetForDialogUid;
    std::function<std::shared_ptr<FriendInfo>(int)> privatePeerById;
    std::function<std::shared_ptr<GroupInfoData>(qint64)> groupById;
    std::function<void(int, const QByteArray&)> dispatchPayload;
    std::function<void(int)> savePersistentDialogStore;
    std::function<void()> refreshDialogModel;
    FriendListModel* groupListModel = nullptr;
};

struct ChatDialogMetaResponseRequest
{
    int reqId = 0;
    QJsonObject payload;
    int currentDialogUid = 0;
    int ownerUid = 0;
    QMap<int, qint64>* groupDialogUidMap = nullptr;
};

struct ChatDialogMetaResponseDependencies
{
    FriendListModel* dialogListModel = nullptr;
    std::function<void(const QString&)> syncCurrentDraftText;
    std::function<void(bool)> syncCurrentDialogMuted;
    std::function<void(bool)> syncCurrentDialogPinned;
    std::function<void(int)> savePersistentDialogStore;
    std::function<void()> refreshDialogModel;
};

struct ChatDialogMetaResponseResult
{
    bool success = false;
    bool draftPath = false;
    bool pinPath = false;
    int dialogUid = 0;
    QString draftText;
    int muteState = 0;
    int pinnedRank = 0;
    bool currentDialog = false;
    bool runtimeUpdated = false;
    bool dialogModelUpdated = false;
    bool currentDraftUpdated = false;
    bool currentMutedUpdated = false;
    bool currentPinnedUpdated = false;
    bool persistentStoreSaved = false;
    bool dialogModelRefreshed = false;
};

struct ChatPrivateConversationClearRequest
{
    int friendUid = 0;
};

struct ChatPrivateConversationClearResult
{
    bool success = false;
    bool skipped = false;
    int friendUid = 0;
    bool currentPrivateConversationCleared = false;
    bool messageModelCleared = false;
    bool peerProjectionReset = false;
    bool runtimeProjectionReset = false;
    bool dialogUidEmitted = false;
};

struct ChatPrivateConversationClearPort
{
    std::function<int()> currentPrivatePeerUid;
    std::function<void(int)> setCurrentPrivatePeerUid;
    std::function<void()> clearMessageModel;
    std::function<void(const QString&)> setCurrentPeerName;
    std::function<void(const QString&)> setCurrentPeerIcon;
    std::function<void(const QString&)> setCurrentDraftText;
    std::function<void(bool)> setCurrentDialogPinned;
    std::function<void(bool)> setCurrentDialogMuted;
    std::function<void()> emitCurrentDialogUidChangedIfNeeded;
};

struct ChatDialogRuntimeState
{
    QString currentDraftText;
    QVariantList currentPendingAttachments;
    QHash<int, QVariantList> pendingAttachmentMap;
    bool currentPinned = false;
    bool currentMuted = false;
    QString replyToMsgId;
    QString replyTargetName;
    QString replyPreviewText;
    QHash<int, QString> draftMap;
    QSet<int> localPinnedSet;
    QHash<int, int> serverPinnedMap;
    QHash<int, int> serverMuteMap;
    QHash<int, int> mentionMap;
    int lastEmittedUid = 0;

    void clear()
    {
        currentDraftText.clear();
        currentPendingAttachments.clear();
        pendingAttachmentMap.clear();
        currentPinned = false;
        currentMuted = false;
        replyToMsgId.clear();
        replyTargetName.clear();
        replyPreviewText.clear();
        draftMap.clear();
        localPinnedSet.clear();
        serverPinnedMap.clear();
        serverMuteMap.clear();
        mentionMap.clear();
        lastEmittedUid = 0;
    }
};

#endif // CHATDIALOGRUNTIMESTATE_H

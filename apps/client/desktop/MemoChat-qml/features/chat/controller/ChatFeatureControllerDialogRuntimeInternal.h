#pragma once

#include "ChatFeatureController.h"

#include "ConversationSyncService.h"
#include "FriendListModel.h"
#include "userdata.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSettings>
#include <QStringList>
#include <QVariantMap>
#include <QtGlobal>

namespace memochat::chat::dialog_runtime
{
inline constexpr int kMaxDraftChars = 2000;
inline constexpr int kMaxReplyPreviewChars = 120;
inline constexpr int kSyncDraftRequestId = 1072;
inline constexpr int kPinDialogRequestId = 1074;
inline constexpr int kSyncDraftResponseId = 1073;
inline constexpr int kPinDialogResponseId = 1075;

inline QString normalizeDraftText(const QString& text)
{
    QString nextDraft = text;
    if (nextDraft.size() > kMaxDraftChars)
    {
        nextDraft = nextDraft.left(kMaxDraftChars);
    }
    return nextDraft.trimmed().isEmpty() ? QString() : nextDraft;
}

inline QString normalizeStoredDraftText(const QString& text)
{
    return text.trimmed().isEmpty() ? QString() : text;
}

inline QString normalizeReplyPreview(const QString& previewText)
{
    QString normalizedPreview = previewText.trimmed();
    if (normalizedPreview.length() > kMaxReplyPreviewChars)
    {
        normalizedPreview = normalizedPreview.left(kMaxReplyPreviewChars);
    }
    return normalizedPreview;
}

inline bool applyDraftToDialogModel(FriendListModel& dialogListModel, int dialogUid, const QString& draftText)
{
    const int idx = dialogListModel.indexOfUid(dialogUid);
    if (idx < 0)
    {
        return false;
    }
    const QVariantMap item = dialogListModel.get(idx);
    dialogListModel.setDialogMeta(
        dialogUid,
        item.value(QStringLiteral("dialogType")).toString(),
                   item.value(QStringLiteral("unreadCount")).toInt(),
                              item.value(QStringLiteral("pinnedRank")).toInt(),
                                         draftText,
                                         item.value(QStringLiteral("lastMsgTs")).toLongLong(),
                                                    item.value(QStringLiteral("muteState")).toInt());
    return true;
}

inline bool applyMuteToDialogModel(FriendListModel& dialogListModel, int dialogUid, int muteState)
{
    const int idx = dialogListModel.indexOfUid(dialogUid);
    if (idx < 0)
    {
        return false;
    }
    const QVariantMap item = dialogListModel.get(idx);
    dialogListModel.setDialogMeta(
        dialogUid,
        item.value(QStringLiteral("dialogType")).toString(),
                   item.value(QStringLiteral("unreadCount")).toInt(),
                              item.value(QStringLiteral("pinnedRank")).toInt(),
                                         item.value(QStringLiteral("draftText")).toString(),
                                                    item.value(QStringLiteral("lastMsgTs")).toLongLong(), muteState);
    return true;
}

inline int dialogUidFromMetaPayload(const QJsonObject& payload, QMap<int, qint64>* groupDialogUidMap)
{
    const QString dialogType = payload.value(QStringLiteral("dialog_type")).toString();
    if (dialogType == QStringLiteral("group"))
    {
        const qint64 groupId = payload.value(QStringLiteral("group_id")).toVariant().toLongLong();
        if (groupId <= 0)
        {
            return 0;
        }
        if (!groupDialogUidMap)
        {
            return ConversationSyncService::makeGroupDialogUid(groupId);
        }
        return ConversationSyncService::resolveGroupDialogUid(*groupDialogUidMap, groupId);
    }
    if (dialogType == QStringLiteral("private"))
    {
        return payload.value(QStringLiteral("peer_uid")).toInt();
    }
    return 0;
}

inline bool invokeSavePersistentDialogStore(int ownerUid, const ChatDialogCommandDependencies& dependencies)
{
    if (ownerUid <= 0 || !dependencies.savePersistentDialogStore)
    {
        return false;
    }
    dependencies.savePersistentDialogStore(ownerUid);
    return true;
}

inline bool
dispatchDialogPayload(int requestId, const QByteArray& payload, const ChatDialogCommandDependencies& dependencies)
{
    if (requestId <= 0 || payload.isEmpty() || !dependencies.dispatchPayload)
    {
        return false;
    }
    dependencies.dispatchPayload(requestId, payload);
    return true;
}

inline ChatDialogCommandDependencies commandDependenciesFromPort(const ChatDialogCommandPort& port)
{
    ChatDialogCommandDependencies dependencies;
    dependencies.dispatchPayload = port.dispatchPayload;
    dependencies.savePersistentDialogStore = port.savePersistentDialogStore;
    dependencies.refreshDialogModel = port.refreshDialogModel;
    return dependencies;
}

inline ChatMarkDialogReadDependencies markReadDependenciesFromPort(const ChatDialogCommandPort& commandPort,
                                                                   const ChatReadAckPort& readAckPort)
{
    ChatMarkDialogReadDependencies dependencies;
    dependencies.groupListModel = commandPort.groupListModel;
    dependencies.readAckDispatchPort = readAckPort.dispatch;
    return dependencies;
}

inline ChatDialogCommandRequest commandRequestForDialog(const ChatDialogCommandSnapshot& snapshot,
                                                        int dialogUid,
                                                        const ChatDialogTarget& target,
                                                        const QString& draftText = QString())
{
    ChatDialogCommandRequest request;
    request.selfUid = snapshot.selfUid;
    request.ownerUid = snapshot.ownerUid;
    request.currentDialogUid = snapshot.currentDialogUid;
    request.dialogUid = dialogUid;
    request.target = target;
    request.draftText = draftText;
    return request;
}

inline ChatDialogTarget currentDialogTarget(const ChatDialogCommandSnapshot& snapshot)
{
    ChatDialogTarget target;
    if (snapshot.currentGroupId > 0)
    {
        target.dialogType = QStringLiteral("group");
        target.groupId = snapshot.currentGroupId;
    }
    else if (snapshot.currentPrivatePeerUid > 0)
    {
        target.dialogType = QStringLiteral("private");
        target.peerUid = snapshot.currentPrivatePeerUid;
    }
    return target;
}

inline ChatDialogTarget targetForDialogUid(const ChatDialogCommandPort& port, int dialogUid)
{
    if (dialogUid > 0)
    {
        ChatDialogTarget target;
        target.dialogType = QStringLiteral("private");
        target.peerUid = dialogUid;
        return target;
    }
    return port.targetForDialogUid ? port.targetForDialogUid(dialogUid) : ChatDialogTarget{};
}

inline qint64 latestPrivatePeerCreatedAt(const ChatDialogCommandPort& port, int peerUid)
{
    if (peerUid <= 0 || !port.privatePeerById)
    {
        return 0;
    }
    const std::shared_ptr<FriendInfo> friendInfo = port.privatePeerById(peerUid);
    if (!friendInfo)
    {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto& message : friendInfo->_chat_msgs)
    {
        if (message && message->_from_uid == peerUid)
        {
            latestTs = qMax(latestTs, message->_created_at);
        }
    }
    return latestTs;
}

inline qint64 latestGroupCreatedAt(const ChatDialogCommandPort& port, qint64 groupId)
{
    if (groupId <= 0 || !port.groupById)
    {
        return 0;
    }
    const std::shared_ptr<GroupInfoData> groupInfo = port.groupById(groupId);
    if (!groupInfo)
    {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto& message : groupInfo->_chat_msgs)
    {
        if (message)
        {
            latestTs = qMax(latestTs, message->_created_at);
        }
    }
    return latestTs;
}
} // namespace memochat::chat::dialog_runtime

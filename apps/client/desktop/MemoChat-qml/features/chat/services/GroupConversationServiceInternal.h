#pragma once

#include "GroupConversationService.h"

#include "ChatMessageModel.h"
#include "ConversationSyncService.h"
#include "DialogListService.h"
#include "FriendListModel.h"
#include "GroupChatCacheStore.h"
#include "MessageContentCodec.h"
#include "MessagePayloadService.h"
#include "userdata.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>
#include <QtGlobal>

namespace memochat::chat::group_conversation
{
inline constexpr int kProtocolSuccess = 0;
inline constexpr int kProtocolJsonError = 1;
inline constexpr int kGroupChatRequestId = 1044;
inline constexpr int kGroupHistoryRequestId = 1047;
inline constexpr int kEditGroupMsgRsp = 1066;
inline constexpr int kRevokeGroupMsgRsp = 1068;
inline constexpr int kForwardGroupMsgRsp = 1070;

inline bool hasUsableState(const GroupConversationDependencies& dependencies)
{
    return dependencies.state != nullptr;
}

inline bool
dispatchIfAvailable(int requestId, const QByteArray& payload, const GroupConversationDependencies& dependencies)
{
    if (requestId <= 0 || payload.isEmpty() || !dependencies.dispatchPayload)
    {
        return false;
    }
    dependencies.dispatchPayload(requestId, payload);
    return true;
}

inline QString senderDisplayName(const GroupConversationContext& context)
{
    return context.selfName.isEmpty() ? QStringLiteral("我") : context.selfName;
}

inline int resolveGroupDialogUid(const GroupConversationDependencies& dependencies, qint64 groupId)
{
    if (!dependencies.dialogUidMap)
    {
        return ConversationSyncService::makeGroupDialogUid(groupId);
    }
    return ConversationSyncService::resolveGroupDialogUid(*dependencies.dialogUidMap, groupId);
}

inline std::shared_ptr<GroupInfoData> groupById(const GroupConversationDependencies& dependencies, qint64 groupId)
{
    if (groupId <= 0 || !dependencies.groupById)
    {
        return nullptr;
    }
    return dependencies.groupById(groupId);
}

inline bool cacheGroupMessages(const GroupConversationDependencies& dependencies,
                               int selfUid,
                               qint64 groupId,
                               const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    if (!dependencies.cacheStore || !dependencies.cacheStore->isReady() || selfUid <= 0 || groupId <= 0 ||
        messages.empty())
    {
        return false;
    }
    dependencies.cacheStore->upsertMessages(selfUid, groupId, messages);
    return true;
}

inline bool cacheWholeGroupIfReady(const GroupConversationDependencies& dependencies,
                                   int selfUid,
                                   qint64 groupId,
                                   const std::shared_ptr<GroupInfoData>& groupInfo)
{
    if (!groupInfo)
    {
        return false;
    }
    return cacheGroupMessages(dependencies, selfUid, groupId, groupInfo->_chat_msgs);
}

inline QString previewFor(const QString& content, const QString& previewText = QString())
{
    return previewText.isEmpty() ? MessageContentCodec::toPreviewText(content) : previewText;
}

inline qint64 latestMessageCreatedAt(const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    qint64 latest = 0;
    for (const auto& one : messages)
    {
        if (one)
        {
            latest = qMax(latest, one->_created_at);
        }
    }
    return latest;
}

inline qint64 smallestGroupSeq(const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    qint64 beforeSeq = 0;
    for (const auto& one : messages)
    {
        if (!one || one->_group_seq <= 0)
        {
            continue;
        }
        if (beforeSeq <= 0 || one->_group_seq < beforeSeq)
        {
            beforeSeq = one->_group_seq;
        }
    }
    return beforeSeq;
}

inline qint64
replyServerMsgIdFor(const GroupConversationDependencies& dependencies, qint64 groupId, const QString& replyToMsgId)
{
    if (replyToMsgId.isEmpty())
    {
        return 0;
    }
    const auto groupInfo = groupById(dependencies, groupId);
    if (!groupInfo)
    {
        return 0;
    }
    for (const auto& one : groupInfo->_chat_msgs)
    {
        if (one && one->_msg_id == replyToMsgId)
        {
            return qMax<qint64>(0, one->_server_msg_id);
        }
    }
    return 0;
}

inline QJsonArray mentionsForPlainText(const QString& plainText)
{
    QJsonArray mentions;
    const QRegularExpression mentionIdRegex(QStringLiteral("@u([1-9][0-9]{8})"));
    QRegularExpressionMatchIterator mentionIt = mentionIdRegex.globalMatch(plainText);
    while (mentionIt.hasNext())
    {
        const QRegularExpressionMatch match = mentionIt.next();
        bool ok = false;
        const int uid = match.captured(1).toInt(&ok);
        if (ok && uid > 0)
        {
            mentions.append(uid);
        }
    }
    return mentions;
}

inline bool mentionsSelf(const GroupChatMsg& msg, int selfUid)
{
    if (selfUid <= 0 || msg._from_uid == selfUid || !msg._msg)
    {
        return false;
    }
    if (msg._msg->_mention_all)
    {
        return true;
    }
    for (const auto& one : msg._msg->_mentions)
    {
        if (one.toInt() == selfUid)
        {
            return true;
        }
    }
    return false;
}

inline void updateGroupPreview(const GroupConversationDependencies& dependencies,
                               int dialogUid,
                               const QString& groupPreview,
                               const QString& dialogPreview,
                               qint64 lastMsgTs,
                               bool* updated)
{
    if (updated)
    {
        *updated = false;
    }
    if (dialogUid == 0 || !dependencies.groupListModel || !dependencies.dialogListModel)
    {
        return;
    }
    ConversationSyncService::updateGroupPreview(*dependencies.groupListModel,
                                                *dependencies.dialogListModel,
                                                dialogUid,
                                                groupPreview,
                                                dialogPreview,
                                                lastMsgTs);
    if (updated)
    {
        *updated = true;
    }
}

inline void clearUnread(const GroupConversationDependencies& dependencies, int dialogUid, bool* cleared)
{
    if (cleared)
    {
        *cleared = false;
    }
    if (!dependencies.dialogListModel || dialogUid == 0 || !dependencies.clearGroupUnreadAndMention)
    {
        return;
    }
    dependencies.clearGroupUnreadAndMention(*dependencies.dialogListModel, dialogUid);
    if (cleared)
    {
        *cleared = true;
    }
}

inline void
incrementUnread(const GroupConversationDependencies& dependencies, int dialogUid, bool mentioned, bool* incremented)
{
    if (incremented)
    {
        *incremented = false;
    }
    if (!dependencies.dialogListModel || dialogUid == 0 || !dependencies.incrementGroupUnreadAndMention)
    {
        return;
    }
    dependencies.incrementGroupUnreadAndMention(*dependencies.dialogListModel, dialogUid, mentioned);
    if (incremented)
    {
        *incremented = true;
    }
}

inline std::shared_ptr<GroupInfoData> ensureGroup(const GroupConversationDependencies& dependencies,
                                                  qint64 groupId,
                                                  const std::shared_ptr<GroupChatMsg>& message,
                                                  int dialogUid,
                                                  bool* created)
{
    if (created)
    {
        *created = false;
    }
    auto groupInfo = groupById(dependencies, groupId);
    if (groupInfo || groupId <= 0 || !dependencies.upsertGroup)
    {
        return groupInfo;
    }

    groupInfo = std::make_shared<GroupInfoData>();
    groupInfo->_group_id = groupId;
    groupInfo->_name = QStringLiteral("群聊%1").arg(groupId);
    groupInfo->_icon = (!message || message->_from_icon.trimmed().isEmpty()) ? QStringLiteral("qrc:/res/chat_icon.png")
                                                                             : message->_from_icon;
    dependencies.upsertGroup(groupInfo);
    groupInfo = groupById(dependencies, groupId);
    if (created)
    {
        *created = true;
    }

    if (groupInfo && dialogUid != 0 && dependencies.groupListModel && dependencies.dialogListModel &&
        dependencies.dialogDecorationState)
    {
        DialogEntrySeed seed;
        seed.dialogUid = dialogUid;
        seed.dialogType = QStringLiteral("group");
        seed.name = groupInfo->_name;
        seed.nick = groupInfo->_name;
        seed.icon = groupInfo->_icon;
        seed.previewText =
            message && message->_msg ? MessageContentCodec::toPreviewText(message->_msg->_msg_content) : QString();
        seed.lastMsgTs = message && message->_msg ? message->_msg->_created_at : QDateTime::currentMSecsSinceEpoch();
        auto item = DialogListService::buildDialogEntry(seed, dependencies.dialogDecorationState());
        dependencies.groupListModel->upsertFriend(item);
        dependencies.dialogListModel->upsertFriend(item);
    }

    return groupInfo;
}

inline QString clientMsgIdFor(const QJsonObject& payload)
{
    QString msgId = payload.value(QStringLiteral("client_msg_id")).toString();
    if (msgId.isEmpty())
    {
        msgId = payload.value(QStringLiteral("msg")).toObject().value(QStringLiteral("msgid")).toString();
    }
    return msgId;
}

inline QString normalizedSenderIcon(const QString& icon)
{
    const QString trimmed = icon.trimmed();
    return trimmed.isEmpty() ? QStringLiteral("qrc:/res/head_1.png") : trimmed;
}

inline QString groupMutationStatusText(int reqId, const QString& event)
{
    if (reqId == kRevokeGroupMsgRsp || event == QStringLiteral("group_msg_revoked"))
    {
        return QStringLiteral("群消息已撤回");
    }
    if (reqId == kEditGroupMsgRsp || event == QStringLiteral("group_msg_edited"))
    {
        return QStringLiteral("群消息已编辑");
    }
    return {};
}

inline QString groupAckStatusText(int reqId)
{
    return reqId == kForwardGroupMsgRsp ? QStringLiteral("消息已转发") : QString();
}

inline QString groupMessageErrorStatusText(const QJsonObject& payload, int errorCode)
{
    const QString serverMessage = payload.value(QStringLiteral("message")).toString().trimmed();
    if (serverMessage.isEmpty())
    {
        return QStringLiteral("发送群消息失败（错误码:%1）").arg(errorCode);
    }
    return QStringLiteral("发送群消息失败：%1（错误码:%2）").arg(serverMessage).arg(errorCode);
}

} // namespace memochat::chat::group_conversation

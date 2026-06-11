#pragma once

#include "PrivateChatEventService.h"

#include "ChatMessageModel.h"
#include "ConversationSyncService.h"
#include "FriendListModel.h"
#include "MessageContentCodec.h"
#include "MessagePayloadService.h"
#include "PrivateChatCacheStore.h"
#include "userdata.h"

#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtGlobal>
#include <utility>

namespace memochat::chat::private_event
{
inline constexpr int kProtocolSuccess = 0;
inline constexpr int kProtocolJsonError = 1;
inline constexpr int kEditPrivateMsgRsp = 1078;
inline constexpr int kRevokePrivateMsgRsp = 1080;
inline constexpr int kForwardPrivateMsgRsp = 1084;

inline bool protocolSucceeded(const QJsonObject& payload)
{
    return payload.value(QStringLiteral("error")).toInt(kProtocolJsonError) == kProtocolSuccess;
}

inline QString privateMutationEventForResponse(int reqId)
{
    if (reqId == kRevokePrivateMsgRsp)
    {
        return QStringLiteral("private_msg_revoked");
    }
    if (reqId == kEditPrivateMsgRsp)
    {
        return QStringLiteral("private_msg_edited");
    }
    return {};
}

inline QString privateMutationStatusText(int reqId)
{
    if (reqId == kRevokePrivateMsgRsp)
    {
        return QStringLiteral("私聊消息已撤回");
    }
    if (reqId == kEditPrivateMsgRsp)
    {
        return QStringLiteral("私聊消息已编辑");
    }
    return QStringLiteral("私聊消息更新失败");
}

inline QString privateResponseActionText(int reqId)
{
    if (reqId == kRevokePrivateMsgRsp)
    {
        return QStringLiteral("撤回私聊消息");
    }
    if (reqId == kEditPrivateMsgRsp)
    {
        return QStringLiteral("编辑私聊消息");
    }
    if (reqId == kForwardPrivateMsgRsp)
    {
        return QStringLiteral("转发私聊消息");
    }
    return {};
}

inline bool isViewingCurrentPrivate(const PrivateChatEventContext& context, int peerUid)
{
    return context.currentGroupId <= 0 && peerUid > 0 && peerUid == context.currentPeerUid;
}

inline int resolvePrivatePeerUid(int selfUid, int fromUid, int peerUidField)
{
    if (fromUid == selfUid)
    {
        return peerUidField;
    }
    if (fromUid > 0)
    {
        return fromUid;
    }
    return peerUidField;
}

inline std::shared_ptr<FriendInfo> lookupFriend(const PrivateChatEventDependencies& dependencies, int peerUid)
{
    if (peerUid <= 0 || !dependencies.friendById)
    {
        return nullptr;
    }
    return dependencies.friendById(peerUid);
}

inline std::shared_ptr<FriendInfo>
ensureFriend(const PrivateChatEventDependencies& dependencies, int peerUid, bool* ensureCalled)
{
    if (ensureCalled)
    {
        *ensureCalled = false;
    }
    std::shared_ptr<FriendInfo> friendInfo = lookupFriend(dependencies, peerUid);
    if (friendInfo || peerUid <= 0 || !dependencies.ensureFriend)
    {
        return friendInfo;
    }

    if (ensureCalled)
    {
        *ensureCalled = true;
    }
    friendInfo = dependencies.ensureFriend(peerUid);
    return friendInfo ? friendInfo : lookupFriend(dependencies, peerUid);
}

inline void normalizeIncomingMessages(const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    for (const auto& one : messages)
    {
        if (!one)
        {
            continue;
        }
        if (one->_deleted_at_ms > 0 || one->_msg_content == QStringLiteral("[消息已撤回]"))
        {
            one->_msg_state = QStringLiteral("deleted");
        }
        else if (one->_edited_at_ms > 0)
        {
            one->_msg_state = QStringLiteral("edited");
        }
    }
}

inline qint64 latestPeerMessageTs(const std::vector<std::shared_ptr<TextChatData>>& messages, int peerUid)
{
    qint64 latestPeerTs = 0;
    for (const auto& one : messages)
    {
        if (one && one->_from_uid == peerUid)
        {
            latestPeerTs = qMax(latestPeerTs, one->_created_at);
        }
    }
    return latestPeerTs;
}

inline std::shared_ptr<TextChatData> latestMessage(const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    for (auto it = messages.crbegin(); it != messages.crend(); ++it)
    {
        if (*it)
        {
            return *it;
        }
    }
    return nullptr;
}

inline bool upsertCacheIfReady(const PrivateChatEventDependencies& dependencies,
                               int selfUid,
                               int peerUid,
                               const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    if (!dependencies.cacheStore || !dependencies.cacheStore->isReady() || selfUid <= 0 || peerUid <= 0 ||
        messages.empty())
    {
        return false;
    }
    dependencies.cacheStore->upsertMessages(selfUid, peerUid, messages);
    return true;
}

inline bool updatePreviewFromMessages(const PrivateChatEventDependencies& dependencies,
                                      int peerUid,
                                      const std::vector<std::shared_ptr<TextChatData>>& friendMessages,
                                      const std::vector<std::shared_ptr<TextChatData>>& fallbackMessages)
{
    if (!dependencies.chatListModel || !dependencies.dialogListModel)
    {
        return false;
    }

    std::shared_ptr<TextChatData> latest = latestMessage(friendMessages);
    if (!latest)
    {
        latest = latestMessage(fallbackMessages);
    }
    if (!latest)
    {
        return false;
    }

    const QString preview = MessageContentCodec::toPreviewText(latest->_msg_content);
    const qint64 lastTs = latest->_created_at > 0 ? latest->_created_at : QDateTime::currentMSecsSinceEpoch();
    ConversationSyncService::updatePrivatePreview(*dependencies.chatListModel,
                                                  *dependencies.dialogListModel,
                                                  peerUid,
                                                  preview,
                                                  lastTs);
    return true;
}

inline std::size_t appendToCurrentModel(const PrivateChatEventDependencies& dependencies,
                                        const std::vector<std::shared_ptr<TextChatData>>& messages,
                                        int selfUid)
{
    if (!dependencies.messageModel)
    {
        return 0;
    }

    const int beforeCount = dependencies.messageModel->rowCount();
    for (const auto& chat : messages)
    {
        dependencies.messageModel->appendMessage(chat, selfUid);
    }
    const int appended = dependencies.messageModel->rowCount() - beforeCount;
    return appended > 0 ? static_cast<std::size_t>(appended) : 0U;
}

inline void syncHistoryCursorIfPossible(const PrivateChatEventDependencies& dependencies)
{
    if (!dependencies.messageModel || !dependencies.historyBeforeTs || !dependencies.historyBeforeMsgId)
    {
        return;
    }
    ConversationSyncService::syncHistoryCursor(*dependencies.messageModel,
                                               *dependencies.historyBeforeTs,
                                               *dependencies.historyBeforeMsgId);
}

inline void upsertFriendMessagesCache(const PrivateChatEventDependencies& dependencies,
                                      int selfUid,
                                      int peerUid,
                                      const std::shared_ptr<FriendInfo>& friendInfo,
                                      bool* cacheUpdated)
{
    if (cacheUpdated)
    {
        *cacheUpdated = false;
    }
    if (!friendInfo)
    {
        return;
    }
    const bool updated = upsertCacheIfReady(dependencies, selfUid, peerUid, friendInfo->_chat_msgs);
    if (cacheUpdated)
    {
        *cacheUpdated = updated;
    }
}
} // namespace memochat::chat::private_event

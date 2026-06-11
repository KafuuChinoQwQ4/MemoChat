#include "PrivateChatSendService.h"

#include "ChatMessageModel.h"
#include "ConversationSyncService.h"
#include "FriendListModel.h"
#include "MessageContentCodec.h"
#include "PrivateChatCacheStore.h"
#include "userdata.h"

#include <QDateTime>
#include <QUuid>

namespace
{
PrivateChatSendResult failedResult(const QString& errorText)
{
    PrivateChatSendResult result;
    result.errorText = errorText;
    return result;
}

std::shared_ptr<TextChatData> buildOptimisticMessage(const OutgoingChatPacket& packet)
{
    return std::make_shared<TextChatData>(packet.msgId,
                                          packet.content,
                                          packet.fromUid,
                                          packet.toUid,
                                          QString(),
                                          packet.createdAt,
                                          QString(),
                                          QStringLiteral("sending"));
}
} // namespace

PrivateChatSendResult PrivateChatSendService::send(const PrivateChatSendRequest& request,
                                                   const PrivateChatSendDependencies& dependencies)
{
    if (request.peerUid <= 0)
    {
        return failedResult(QStringLiteral("未选择聊天对象"));
    }
    if (dependencies.selfUid <= 0)
    {
        return failedResult(QStringLiteral("当前用户不可用"));
    }
    if (!dependencies.dispatchPacket)
    {
        return failedResult(QStringLiteral("消息发送器不可用"));
    }

    PrivateChatSendResult result;
    result.packet.fromUid = dependencies.selfUid;
    result.packet.toUid = request.peerUid;
    result.packet.msgId = QUuid::createUuid().toString();
    result.packet.content = request.content;
    result.packet.createdAt = QDateTime::currentMSecsSinceEpoch();

    QString dispatchError;
    if (!dependencies.dispatchPacket(result.packet, &dispatchError))
    {
        result.errorText = dispatchError.isEmpty() ? QStringLiteral("消息发送失败") : dispatchError;
        return result;
    }

    result.optimisticMessage = buildOptimisticMessage(result.packet);
    const std::vector<std::shared_ptr<TextChatData>> optimisticMessages{result.optimisticMessage};

    if (dependencies.appendFriendMessages)
    {
        dependencies.appendFriendMessages(request.peerUid, optimisticMessages);
    }
    if (dependencies.cacheStore && dependencies.cacheStore->isReady())
    {
        dependencies.cacheStore->upsertMessages(dependencies.selfUid, request.peerUid, optimisticMessages);
    }
    if (dependencies.messageModel)
    {
        dependencies.messageModel->appendMessage(result.optimisticMessage, dependencies.selfUid);
        if (dependencies.historyBeforeTs && dependencies.historyBeforeMsgId)
        {
            ConversationSyncService::syncHistoryCursor(*dependencies.messageModel,
                                                       *dependencies.historyBeforeTs,
                                                       *dependencies.historyBeforeMsgId);
        }
    }
    if (dependencies.chatListModel && dependencies.dialogListModel)
    {
        const QString resolvedPreview =
            request.previewText.isEmpty() ? MessageContentCodec::toPreviewText(request.content) : request.previewText;
        ConversationSyncService::updatePrivatePreview(*dependencies.chatListModel,
                                                      *dependencies.dialogListModel,
                                                      request.peerUid,
                                                      resolvedPreview,
                                                      result.packet.createdAt);
        ConversationSyncService::clearPrivateUnread(*dependencies.chatListModel,
                                                    *dependencies.dialogListModel,
                                                    request.peerUid);
    }

    result.success = true;
    return result;
}

bool PrivateChatSendService::trySend(const PrivateChatSendRequest& request,
                                     const PrivateChatSendDependencies& dependencies,
                                     QString* errorText)
{
    const PrivateChatSendResult result = send(request, dependencies);
    if (errorText)
    {
        *errorText = result.errorText;
    }
    return result.success;
}

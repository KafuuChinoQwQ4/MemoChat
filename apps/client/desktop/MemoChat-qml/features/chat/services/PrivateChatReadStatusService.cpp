#include "PrivateChatEventServiceInternal.h"

using namespace memochat::chat::private_event;

PrivateReadAckResult PrivateChatEventService::handleReadAck(const PrivateReadAckRequest& request,
                                                            const PrivateChatEventDependencies& dependencies)
{
    PrivateReadAckResult result;
    if (request.context.selfUid <= 0 || !protocolSucceeded(request.payload))
    {
        return result;
    }

    result.peerUid = request.payload.value(QStringLiteral("fromuid")).toInt();
    if (result.peerUid <= 0 || result.peerUid == request.context.selfUid || !dependencies.markOutgoingReadUntil)
    {
        return result;
    }

    result.readTs = MessagePayloadService::normalizeEpochMs(
        request.payload.value(QStringLiteral("read_ts")).toVariant().toLongLong(), true);
    result.updatedCount = dependencies.markOutgoingReadUntil(result.peerUid, request.context.selfUid, result.readTs);
    if (result.updatedCount <= 0)
    {
        return result;
    }

    const std::shared_ptr<FriendInfo> friendInfo = lookupFriend(dependencies, result.peerUid);
    upsertFriendMessagesCache(dependencies, request.context.selfUid, result.peerUid, friendInfo, &result.cacheUpdated);

    if (friendInfo && isViewingCurrentPrivate(request.context, result.peerUid) && dependencies.messageModel)
    {
        for (const auto& one : friendInfo->_chat_msgs)
        {
            if (!one || one->_from_uid != request.context.selfUid || one->_created_at > result.readTs)
            {
                continue;
            }
            if (one->_msg_state != QStringLiteral("read"))
            {
                continue;
            }
            dependencies.messageModel->updateMessageState(one->_msg_id, QStringLiteral("read"));
            ++result.modelPatchCount;
        }
    }

    result.success = true;
    return result;
}

PrivateMessageStatusResult
PrivateChatEventService::handleMessageStatus(const PrivateMessageStatusRequest& request,
                                             const PrivateChatEventDependencies& dependencies)
{
    PrivateMessageStatusResult result;
    result.clientMsgId = request.payload.value(QStringLiteral("client_msg_id")).toString();
    result.nextState = resolveMessageStatusState(request.payload);
    if (result.clientMsgId.isEmpty() || !dependencies.updatePrivateMessageState)
    {
        return result;
    }

    result.peerUid =
        request.payload.value(QStringLiteral("peer_uid")).toInt(request.payload.value(QStringLiteral("touid")).toInt());
    if (result.peerUid <= 0)
    {
        return result;
    }

    result.userManagerUpdated =
        dependencies.updatePrivateMessageState(result.peerUid, result.clientMsgId, result.nextState);
    if (result.userManagerUpdated && isViewingCurrentPrivate(request.context, result.peerUid) &&
        dependencies.messageModel)
    {
        dependencies.messageModel->updateMessageState(result.clientMsgId, result.nextState);
        result.modelPatched = true;
    }

    const std::shared_ptr<FriendInfo> friendInfo = lookupFriend(dependencies, result.peerUid);
    upsertFriendMessagesCache(dependencies, request.context.selfUid, result.peerUid, friendInfo, &result.cacheUpdated);

    result.success = true;
    return result;
}

QByteArray PrivateChatEventService::buildReadAckPayload(int selfUid, int peerUid, qint64 readTs)
{
    if (selfUid <= 0 || peerUid <= 0)
    {
        return {};
    }
    const qint64 normalizedReadTs = readTs <= 0 ? QDateTime::currentMSecsSinceEpoch() : readTs;
    QJsonObject payloadObj;
    payloadObj[QStringLiteral("fromuid")] = selfUid;
    payloadObj[QStringLiteral("peer_uid")] = peerUid;
    payloadObj[QStringLiteral("read_ts")] = static_cast<qint64>(normalizedReadTs);
    return QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
}

QString PrivateChatEventService::resolveMessageStatusState(const QJsonObject& payload)
{
    return MessagePayloadService::resolveMessageStatusState(payload);
}

#include "PrivateChatHistoryService.h"

#include "ChatMessageModel.h"
#include "ConversationSyncService.h"
#include "FriendListModel.h"
#include "MessageContentCodec.h"
#include "MessagePayloadService.h"
#include "PrivateChatCacheStore.h"

#include <QJsonArray>
#include <QtGlobal>
#include <algorithm>

namespace
{
constexpr int kProtocolSuccess = 0;
constexpr int kProtocolJsonError = 1;

bool isPendingResponse(int peerUid, const PrivateHistoryRequestState& state)
{
    return state.pendingPeerUid > 0 && peerUid == state.pendingPeerUid;
}

void clearPending(PrivateHistoryRequestState& state)
{
    state.pendingBeforeTs = 0;
    state.pendingBeforeMsgId.clear();
    state.pendingPeerUid = 0;
}

void cleanupPendingFailure(const PrivateHistoryResponseDependencies& dependencies,
                           PrivateHistoryRequestState* state,
                           bool pendingMatched)
{
    if (!pendingMatched || !state)
    {
        return;
    }
    clearPending(*state);
    if (dependencies.setCanLoadMore)
    {
        dependencies.setCanLoadMore(false);
    }
}

std::vector<std::shared_ptr<TextChatData>> parseHistoryMessages(const QJsonArray& messages)
{
    std::vector<std::shared_ptr<TextChatData>> parsed;
    parsed.reserve(messages.size());
    for (const auto& one : messages)
    {
        auto msg = MessagePayloadService::buildPrivateHistoryMessage(one.toObject());
        if (msg)
        {
            parsed.push_back(msg);
        }
    }
    std::sort(parsed.begin(),
              parsed.end(),
              [](const std::shared_ptr<TextChatData>& lhs, const std::shared_ptr<TextChatData>& rhs)
              {
                  if (!lhs || !rhs)
                  {
                      return static_cast<bool>(lhs);
                  }
                  if (lhs->_created_at != rhs->_created_at)
                  {
                      return lhs->_created_at < rhs->_created_at;
                  }
                  return lhs->_msg_id < rhs->_msg_id;
              });
    return parsed;
}

qint64 latestPeerMessageTs(const std::vector<std::shared_ptr<TextChatData>>& messages, int peerUid)
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

void updatePreviewFromFriendMessages(const PrivateHistoryResponseDependencies& dependencies,
                                     int peerUid,
                                     const std::vector<std::shared_ptr<TextChatData>>& friendMessages)
{
    if (friendMessages.empty() || !friendMessages.back() || !dependencies.chatListModel ||
        !dependencies.dialogListModel)
    {
        return;
    }

    const QString preview = MessageContentCodec::toPreviewText(friendMessages.back()->_msg_content);
    const qint64 lastTs = friendMessages.back()->_created_at;
    ConversationSyncService::updatePrivatePreview(*dependencies.chatListModel,
                                                  *dependencies.dialogListModel,
                                                  peerUid,
                                                  preview,
                                                  lastTs);
}
} // namespace

PrivateHistoryResponseResult
PrivateChatHistoryService::handleResponse(const PrivateHistoryResponseRequest& request,
                                          const PrivateHistoryResponseDependencies& dependencies)
{
    PrivateHistoryResponseResult result;
    result.peerUid = request.payload.value(QStringLiteral("peer_uid")).toInt();

    PrivateHistoryRequestState* state = dependencies.historyState;
    result.pendingMatched = state ? isPendingResponse(result.peerUid, *state) : false;
    if (result.pendingMatched && dependencies.setLoading)
    {
        dependencies.setLoading(false);
    }

    const int error = request.payload.value(QStringLiteral("error")).toInt(kProtocolJsonError);
    if (error != kProtocolSuccess || request.selfUid <= 0)
    {
        cleanupPendingFailure(dependencies, state, result.pendingMatched);
        return result;
    }

    const bool hasMore = request.payload.value(QStringLiteral("has_more")).toBool(false);
    result.canLoadMore = hasMore;
    const std::vector<std::shared_ptr<TextChatData>> parsed =
        parseHistoryMessages(request.payload.value(QStringLiteral("messages")).toArray());
    result.parsedCount = parsed.size();

    if (!parsed.empty())
    {
        if (dependencies.cacheStore && dependencies.cacheStore->isReady())
        {
            dependencies.cacheStore->upsertMessages(request.selfUid, result.peerUid, parsed);
        }
        if (dependencies.appendFriendMessages)
        {
            dependencies.appendFriendMessages(result.peerUid, parsed);
        }
    }

    const std::vector<std::shared_ptr<TextChatData>> friendMessages =
        dependencies.friendMessages ? dependencies.friendMessages(result.peerUid)
                                    : std::vector<std::shared_ptr<TextChatData>>{};
    updatePreviewFromFriendMessages(dependencies, result.peerUid, friendMessages);

    result.currentDialog = result.peerUid == request.currentPeerUid;
    if (result.currentDialog && dependencies.messageModel)
    {
        result.pagination =
            result.pendingMatched && state && (state->pendingBeforeTs > 0 || !state->pendingBeforeMsgId.isEmpty());
        if (result.pagination)
        {
            dependencies.messageModel->prependMessages(parsed, request.selfUid);
        }
        else if (!friendMessages.empty())
        {
            dependencies.messageModel->setMessages(friendMessages, request.selfUid);
        }
        else
        {
            dependencies.messageModel->clear();
        }

        if (state)
        {
            ConversationSyncService::syncHistoryCursor(*dependencies.messageModel, state->beforeTs, state->beforeMsgId);
        }
        if (dependencies.setCanLoadMore)
        {
            dependencies.setCanLoadMore(hasMore);
        }

        result.requestedReadAckTs = latestPeerMessageTs(friendMessages, result.peerUid);
        if (result.requestedReadAckTs > 0 && dependencies.requestReadAck)
        {
            dependencies.requestReadAck(result.peerUid, result.requestedReadAckTs);
        }
    }

    if (result.pendingMatched && state)
    {
        clearPending(*state);
    }

    result.success = true;
    return result;
}

#include "PrivateChatHistoryRequestService.h"

#include "ChatMessageModel.h"
#include "ConversationSyncService.h"

#include <QJsonDocument>
#include <QtGlobal>

namespace
{
constexpr int kPrivateHistoryRequestId = 1059;

QString trimmedMsgId(const QString& msgId)
{
    return msgId.trimmed();
}

QJsonObject makePayload(int selfUid, int peerUid, qint64 beforeTs, const QString& beforeMsgId)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("fromuid"), selfUid);
    payload.insert(QStringLiteral("peer_uid"), peerUid);
    payload.insert(QStringLiteral("before_ts"), beforeTs);
    const QString trimmedBeforeMsgId = trimmedMsgId(beforeMsgId);
    if (!trimmedBeforeMsgId.isEmpty())
    {
        payload.insert(QStringLiteral("before_msg_id"), trimmedBeforeMsgId);
    }
    payload.insert(QStringLiteral("limit"), PrivateChatHistoryRequestService::kHistoryLimit);
    return payload;
}

QByteArray compactPayload(const QJsonObject& payload)
{
    return QJsonDocument(payload).toJson(QJsonDocument::Compact);
}

void dispatchIfAvailable(int requestId,
                         const QByteArray& payload,
                         const std::function<void(int, const QByteArray&)>& dispatchPayload,
                         bool& dispatched)
{
    dispatched = false;
    if (requestId <= 0 || payload.isEmpty() || !dispatchPayload)
    {
        return;
    }
    dispatchPayload(requestId, payload);
    dispatched = true;
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

void setLoadingFalseAndDisableMore(const PrivateHistoryLoadCurrentDependencies& dependencies)
{
    if (dependencies.setLoading)
    {
        dependencies.setLoading(false);
    }
    if (dependencies.setCanLoadMore)
    {
        dependencies.setCanLoadMore(false);
    }
}

void clearCurrentPrivateState(const PrivateHistoryLoadCurrentDependencies& dependencies)
{
    if (dependencies.historyState)
    {
        PrivateChatHistoryRequestService::clearState(*dependencies.historyState);
    }
    if (dependencies.messageModel)
    {
        dependencies.messageModel->clear();
    }
    setLoadingFalseAndDisableMore(dependencies);
}

QString resolvedPeerIcon(const QString& icon, const QString& defaultIcon)
{
    const QString trimmedIcon = icon.trimmed();
    if (!trimmedIcon.isEmpty())
    {
        return icon;
    }
    const QString trimmedDefaultIcon = defaultIcon.trimmed();
    if (!trimmedDefaultIcon.isEmpty())
    {
        return defaultIcon;
    }
    return QStringLiteral("qrc:/res/head_1.png");
}

PrivateHistoryLoadCurrentResult failedLoadCurrentResult(int peerUid,
                                                        int selfUid,
                                                        const QString& errorText,
                                                        const PrivateHistoryLoadCurrentDependencies& dependencies)
{
    clearCurrentPrivateState(dependencies);

    PrivateHistoryLoadCurrentResult result;
    result.cleared = true;
    result.peerUid = peerUid;
    result.selfUid = selfUid;
    result.errorText = errorText;
    return result;
}

PrivateHistoryRequestBuildResult failedBuildResult(const PrivateHistoryRequestBuildRequest& request,
                                                   const QString& errorText)
{
    PrivateHistoryRequestBuildResult result;
    result.peerUid = request.peerUid;
    result.selfUid = request.selfUid;
    result.beforeTs = request.beforeTs;
    result.beforeMsgId = trimmedMsgId(request.beforeMsgId);
    result.errorText = errorText;
    return result;
}
} // namespace

void PrivateChatHistoryRequestService::clearState(PrivateHistoryRequestState& state)
{
    state.beforeTs = 0;
    state.beforeMsgId.clear();
    clearPending(state);
}

void PrivateChatHistoryRequestService::clearPending(PrivateHistoryRequestState& state)
{
    state.pendingBeforeTs = 0;
    state.pendingBeforeMsgId.clear();
    state.pendingPeerUid = 0;
}

PrivateHistoryLoadCurrentResult
PrivateChatHistoryRequestService::loadCurrent(const PrivateHistoryLoadCurrentRequest& request,
                                              const PrivateHistoryLoadCurrentDependencies& dependencies)
{
    if (!dependencies.historyState)
    {
        return failedLoadCurrentResult(request.currentPeerUid,
                                       request.selfUid,
                                       QStringLiteral("private history state unavailable"), dependencies);
    }
    if (!dependencies.messageModel)
    {
        clearState(*dependencies.historyState);
        setLoadingFalseAndDisableMore(dependencies);
        PrivateHistoryLoadCurrentResult result;
        result.cleared = true;
        result.peerUid = request.currentPeerUid;
        result.selfUid = request.selfUid;
        result.errorText = QStringLiteral("message model unavailable");
        return result;
    }
    if (request.currentPeerUid <= 0)
    {
        return failedLoadCurrentResult(request.currentPeerUid,
                                       request.selfUid,
                                       QStringLiteral("no current private peer"), dependencies);
    }
    if (request.selfUid <= 0)
    {
        return failedLoadCurrentResult(request.currentPeerUid,
                                       request.selfUid,
                                       QStringLiteral("current user unavailable"), dependencies);
    }
    if (!dependencies.peerProfile)
    {
        return failedLoadCurrentResult(request.currentPeerUid,
                                       request.selfUid,
                                       QStringLiteral("peer profile callback unavailable"), dependencies);
    }
    if (!dependencies.friendMessages)
    {
        return failedLoadCurrentResult(request.currentPeerUid,
                                       request.selfUid,
                                       QStringLiteral("friend messages callback unavailable"), dependencies);
    }
    if (dependencies.loadRecentMessages && !dependencies.appendFriendMessages)
    {
        return failedLoadCurrentResult(request.currentPeerUid,
                                       request.selfUid,
                                       QStringLiteral("append friend messages callback unavailable"), dependencies);
    }

    PrivateHistoryPeerProfile peerProfile = dependencies.peerProfile(request.currentPeerUid);
    if (!peerProfile.available)
    {
        return failedLoadCurrentResult(request.currentPeerUid,
                                       request.selfUid,
                                       QStringLiteral("private peer unavailable"), dependencies);
    }

    if (dependencies.setCurrentPeerName)
    {
        dependencies.setCurrentPeerName(peerProfile.displayName);
    }
    if (dependencies.setCurrentPeerIcon)
    {
        dependencies.setCurrentPeerIcon(resolvedPeerIcon(peerProfile.icon, dependencies.defaultPeerIcon));
    }

    const int localLimit = request.localLimit > 0 ? request.localLimit : kHistoryLimit;
    std::vector<std::shared_ptr<TextChatData>> localMessages;
    if (dependencies.loadRecentMessages)
    {
        localMessages = dependencies.loadRecentMessages(request.selfUid, request.currentPeerUid, localLimit);
        if (!localMessages.empty() && dependencies.appendFriendMessages)
        {
            dependencies.appendFriendMessages(request.currentPeerUid, localMessages);
        }
    }

    peerProfile = dependencies.peerProfile(request.currentPeerUid);
    if (!peerProfile.available)
    {
        return failedLoadCurrentResult(request.currentPeerUid,
                                       request.selfUid,
                                       QStringLiteral("private peer unavailable after local merge"), dependencies);
    }

    std::vector<std::shared_ptr<TextChatData>> messages = dependencies.friendMessages(request.currentPeerUid);

    dependencies.messageModel->setMessages(messages, request.selfUid);
    ConversationSyncService::syncHistoryCursor(*dependencies.messageModel,
                                               dependencies.historyState->beforeTs,
                                               dependencies.historyState->beforeMsgId);
    clearPending(*dependencies.historyState);

    if (dependencies.setLoading)
    {
        dependencies.setLoading(false);
    }
    if (dependencies.setCanLoadMore)
    {
        dependencies.setCanLoadMore(true);
    }

    PrivateHistoryLoadCurrentResult result;
    result.success = true;
    result.shouldRequestInitialHistory = true;
    result.peerUid = request.currentPeerUid;
    result.selfUid = request.selfUid;
    result.localCacheCount = static_cast<int>(localMessages.size());
    result.friendMessageCount = static_cast<int>(messages.size());
    result.beforeTs = dependencies.historyState->beforeTs;
    result.beforeMsgId = dependencies.historyState->beforeMsgId;
    result.requestedReadAckTs = latestPeerMessageTs(messages, request.currentPeerUid);
    if (result.requestedReadAckTs > 0 && dependencies.requestReadAck)
    {
        dependencies.requestReadAck(request.currentPeerUid, result.requestedReadAckTs);
    }
    return result;
}

PrivateHistoryRequestBuildResult
PrivateChatHistoryRequestService::buildRequest(const PrivateHistoryRequestBuildRequest& request,
                                               const PrivateHistoryRequestBuildDependencies& dependencies)
{
    if (request.privateHistoryLoading)
    {
        return failedBuildResult(request, QStringLiteral("private history is already loading"));
    }
    if (request.peerUid <= 0)
    {
        return failedBuildResult(request, QStringLiteral("invalid private peer uid"));
    }
    if (request.selfUid <= 0)
    {
        return failedBuildResult(request, QStringLiteral("current user unavailable"));
    }
    if (!dependencies.historyState)
    {
        return failedBuildResult(request, QStringLiteral("private history state unavailable"));
    }

    PrivateHistoryRequestBuildResult result;
    result.success = true;
    result.peerUid = request.peerUid;
    result.selfUid = request.selfUid;
    result.beforeTs = request.beforeTs;
    result.beforeMsgId = trimmedMsgId(request.beforeMsgId);
    result.payload = makePayload(request.selfUid, request.peerUid, request.beforeTs, result.beforeMsgId);
    result.compactPayload = compactPayload(result.payload);

    dependencies.historyState->pendingPeerUid = request.peerUid;
    dependencies.historyState->pendingBeforeTs = request.beforeTs;
    dependencies.historyState->pendingBeforeMsgId = result.beforeMsgId;
    result.pendingUpdated = true;

    if (dependencies.setLoading)
    {
        dependencies.setLoading(true);
        result.loadingSet = true;
    }

    dispatchIfAvailable(kPrivateHistoryRequestId,
                        result.compactPayload,
                        dependencies.dispatchPayload,
                        result.dispatched);
    return result;
}

PrivateHistoryRequestBuildResult
PrivateChatHistoryRequestService::buildBootstrapRequest(const PrivateHistoryBootstrapRequest& request,
                                                        const PrivateHistoryBootstrapDependencies& dependencies)
{
    PrivateHistoryRequestBuildRequest buildRequestInput;
    buildRequestInput.peerUid = request.peerUid;
    buildRequestInput.selfUid = request.selfUid;
    buildRequestInput.beforeTs = 0;

    if (request.peerUid <= 0)
    {
        return failedBuildResult(buildRequestInput, QStringLiteral("invalid private peer uid"));
    }
    if (request.selfUid <= 0)
    {
        return failedBuildResult(buildRequestInput, QStringLiteral("current user unavailable"));
    }

    PrivateHistoryRequestBuildResult result;
    result.success = true;
    result.peerUid = request.peerUid;
    result.selfUid = request.selfUid;
    result.beforeTs = 0;
    result.payload = makePayload(request.selfUid, request.peerUid, 0, QString());
    result.compactPayload = compactPayload(result.payload);
    dispatchIfAvailable(kPrivateHistoryRequestId,
                        result.compactPayload,
                        dependencies.dispatchPayload,
                        result.dispatched);
    return result;
}

PrivateHistoryLoadMoreCursor
PrivateChatHistoryRequestService::selectLoadMoreCursor(const PrivateHistoryLoadMoreRequest& request,
                                                       const PrivateHistoryLoadMoreDependencies& dependencies)
{
    PrivateHistoryLoadMoreCursor cursor;
    cursor.peerUid = request.currentPeerUid;
    if (request.privateHistoryLoading)
    {
        cursor.errorText = QStringLiteral("private history is already loading");
        return cursor;
    }
    if (!request.canLoadMorePrivateHistory)
    {
        cursor.errorText = QStringLiteral("private history has no more messages");
        return cursor;
    }
    if (request.currentPeerUid <= 0)
    {
        cursor.errorText = QStringLiteral("invalid private peer uid");
        return cursor;
    }

    if (dependencies.messageModel)
    {
        const qint64 modelBeforeTs = dependencies.messageModel->earliestCreatedAt();
        if (modelBeforeTs > 0)
        {
            cursor.beforeTs = modelBeforeTs;
            cursor.beforeMsgId = dependencies.messageModel->earliestMsgId();
            cursor.usedModelCursor = true;
            cursor.shouldRequest = true;
            return cursor;
        }
    }

    if (dependencies.historyState)
    {
        cursor.beforeTs = dependencies.historyState->beforeTs;
        cursor.beforeMsgId = dependencies.historyState->beforeMsgId;
        cursor.usedStateCursor = true;
    }
    cursor.shouldRequest = true;
    return cursor;
}

PrivateHistoryRequestBuildResult
PrivateChatHistoryRequestService::buildLoadMoreRequest(const PrivateHistoryLoadMoreRequest& request,
                                                       const PrivateHistoryLoadMoreDependencies& dependencies)
{
    const PrivateHistoryLoadMoreCursor cursor = selectLoadMoreCursor(request, dependencies);
    if (!cursor.shouldRequest)
    {
        PrivateHistoryRequestBuildRequest buildRequestInput;
        buildRequestInput.peerUid = request.currentPeerUid;
        buildRequestInput.selfUid = request.selfUid;
        buildRequestInput.privateHistoryLoading = request.privateHistoryLoading;
        return failedBuildResult(buildRequestInput, cursor.errorText);
    }

    PrivateHistoryRequestBuildRequest buildRequestInput;
    buildRequestInput.peerUid = cursor.peerUid;
    buildRequestInput.selfUid = request.selfUid;
    buildRequestInput.beforeTs = cursor.beforeTs;
    buildRequestInput.beforeMsgId = cursor.beforeMsgId;
    buildRequestInput.privateHistoryLoading = request.privateHistoryLoading;

    PrivateHistoryRequestBuildDependencies buildDependencies;
    buildDependencies.historyState = dependencies.historyState;
    buildDependencies.setLoading = dependencies.setLoading;
    buildDependencies.dispatchPayload = dependencies.dispatchPayload;
    return PrivateChatHistoryRequestService::buildRequest(buildRequestInput, buildDependencies);
}

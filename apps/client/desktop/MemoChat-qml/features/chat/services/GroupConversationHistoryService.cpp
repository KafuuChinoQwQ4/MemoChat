#include "GroupConversationServiceInternal.h"

using namespace memochat::chat::group_conversation;

GroupHistoryBuildResult GroupConversationService::buildHistoryRequest(const GroupHistoryBuildRequest& request,
                                                                      const GroupConversationDependencies& dependencies)
{
    GroupHistoryBuildResult result;
    if (!hasUsableState(dependencies) || request.selfUid <= 0 || request.groupId <= 0)
    {
        return result;
    }
    if (!request.bootstrap)
    {
        if (request.loading || (!dependencies.state->historyHasMore && dependencies.state->historyBeforeSeq > 0))
        {
            result.skipped = true;
            return result;
        }
        result.beforeSeq = dependencies.state->historyBeforeSeq;
    }

    QJsonObject obj;
    obj[QStringLiteral("fromuid")] = request.selfUid;
    obj[QStringLiteral("groupid")] = static_cast<qint64>(request.groupId);
    obj[QStringLiteral("before_ts")] = static_cast<qint64>(0);
    obj[QStringLiteral("before_seq")] = static_cast<qint64>(result.beforeSeq);
    obj[QStringLiteral("limit")] = request.limit <= 0 ? 50 : request.limit;
    result.compactPayload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    result.dispatched = dispatchIfAvailable(kGroupHistoryRequestId, result.compactPayload, dependencies);
    result.success = true;
    return result;
}

GroupHistoryRequestResult GroupConversationService::requestHistory(const GroupHistoryRequestCommand& request,
                                                                   const GroupConversationDependencies& dependencies,
                                                                   const GroupHistoryRequestPort& port)
{
    GroupHistoryRequestResult result;
    const qint64 targetGroupId = request.targetGroupId > 0 ? request.targetGroupId : request.currentGroupId;
    result.groupId = targetGroupId;
    result.currentGroupId = request.currentGroupId;
    result.currentDialog = targetGroupId > 0 && targetGroupId == request.currentGroupId;

    if (!hasUsableState(dependencies) || request.selfUid <= 0 || targetGroupId <= 0)
    {
        return result;
    }

    if (result.currentDialog && request.currentGroupLoading)
    {
        result.skipped = true;
        result.skippedByLoading = true;
        return result;
    }

    if (!request.bootstrap && !dependencies.state->historyHasMore && dependencies.state->historyBeforeSeq > 0)
    {
        result.skipped = true;
        result.skippedByNoMore = true;
        return result;
    }

    GroupHistoryBuildRequest buildRequest;
    buildRequest.selfUid = request.selfUid;
    buildRequest.groupId = targetGroupId;
    buildRequest.loading = false;
    buildRequest.limit = request.limit;
    buildRequest.bootstrap = request.bootstrap;

    const GroupHistoryBuildResult buildResult = buildHistoryRequest(buildRequest, dependencies);
    result.success = buildResult.success;
    result.skipped = buildResult.skipped;
    result.compactPayload = buildResult.compactPayload;
    result.beforeSeq = buildResult.beforeSeq;
    result.dispatched = buildResult.dispatched;
    if (!result.success || result.skipped)
    {
        return result;
    }

    if (result.currentDialog)
    {
        if (port.setGroupHistoryLoading)
        {
            port.setGroupHistoryLoading(true);
        }
        if (port.setPrivateHistoryLoading)
        {
            port.setPrivateHistoryLoading(true);
        }
        result.loadingProjected = true;
    }
    return result;
}

GroupOpenResult GroupConversationService::openGroupConversation(const GroupOpenRequest& request,
                                                                const GroupConversationDependencies& dependencies)
{
    GroupOpenResult result;
    result.groupId = request.groupId;
    if (!hasUsableState(dependencies) || request.context.selfUid <= 0 || request.groupId <= 0)
    {
        return result;
    }

    if (request.resetHistory)
    {
        dependencies.state->historyBeforeSeq = 0;
        dependencies.state->historyHasMore = true;
    }

    auto groupInfo = groupById(dependencies, request.groupId);
    if (dependencies.cacheStore && dependencies.cacheStore->isReady())
    {
        const int limit = request.cacheLimit <= 0 ? 50 : request.cacheLimit;
        const auto localMessages =
            dependencies.cacheStore->loadRecentMessages(request.context.selfUid, request.groupId, limit);
        for (const auto& one : localMessages)
        {
            if (dependencies.upsertGroupMessage)
            {
                dependencies.upsertGroupMessage(request.groupId, one);
            }
        }
        result.cacheLoaded = !localMessages.empty();
        groupInfo = groupById(dependencies, request.groupId);
    }

    if (!groupInfo)
    {
        if (dependencies.messageModel)
        {
            dependencies.messageModel->clear();
            result.modelCleared = true;
        }
        result.success = true;
        return result;
    }

    if (dependencies.messageModel)
    {
        dependencies.messageModel->setMessages(groupInfo->_chat_msgs, request.context.selfUid);
        result.modelSet = true;
    }
    result.requestedReadAckTs = latestMessageCreatedAt(groupInfo->_chat_msgs);
    result.success = true;
    return result;
}

GroupHistoryResponseResult
GroupConversationService::handleHistoryResponse(const GroupHistoryResponseRequest& request,
                                                const GroupConversationDependencies& dependencies,
                                                const GroupHistoryResponsePort& port)
{
    GroupHistoryResponseResult result;
    result.currentGroupId = request.context.currentGroupId;
    result.groupId = request.payload.value(QStringLiteral("groupid")).toVariant().toLongLong();
    result.currentDialog = result.groupId > 0 && result.groupId == request.context.currentGroupId;
    result.messageCount = request.payload.value(QStringLiteral("messages")).toArray().size();

    const auto applyEffects = [&port, &result]()
    {
        if (result.currentDialog)
        {
            if (port.setGroupHistoryLoading)
            {
                port.setGroupHistoryLoading(false);
            }
            if (port.setPrivateHistoryLoading)
            {
                port.setPrivateHistoryLoading(false);
            }
            result.loadingCleared = true;
        }
        if (port.setCanLoadMorePrivateHistory)
        {
            port.setCanLoadMorePrivateHistory(result.currentDialog && result.canLoadMore);
            result.canLoadMoreProjected = true;
        }
        if (result.currentDialog && result.success)
        {
            if (port.sendGroupReadAck)
            {
                port.sendGroupReadAck(result.groupId, result.requestedReadAckTs);
                result.readAckRequested = true;
            }
            if (port.setGroupStatus)
            {
                port.setGroupStatus(QStringLiteral("历史消息已加载"), false);
                result.statusSet = true;
            }
        }
    };

    if (!hasUsableState(dependencies) || request.context.selfUid <= 0)
    {
        applyEffects();
        return result;
    }

    if (result.groupId <= 0)
    {
        applyEffects();
        return result;
    }

    auto groupInfo = groupById(dependencies, result.groupId);
    result.cachedMessageCount = groupInfo ? static_cast<int>(groupInfo->_chat_msgs.size()) : 0;
    result.cacheUpdated = cacheWholeGroupIfReady(dependencies, request.context.selfUid, result.groupId, groupInfo);
    if (!result.currentDialog)
    {
        result.success = true;
        applyEffects();
        return result;
    }

    result.canLoadMore = request.payload.value(QStringLiteral("has_more")).toBool(false);
    dependencies.state->historyHasMore = result.canLoadMore;
    dependencies.state->historyBeforeSeq =
        request.payload.value(QStringLiteral("next_before_seq")).toVariant().toLongLong();

    if (groupInfo && dependencies.messageModel)
    {
        dependencies.messageModel->setMessages(groupInfo->_chat_msgs, request.context.selfUid);
        result.modelSet = true;
        if (dependencies.state->historyBeforeSeq <= 0)
        {
            dependencies.state->historyBeforeSeq = smallestGroupSeq(groupInfo->_chat_msgs);
        }
        result.requestedReadAckTs = latestMessageCreatedAt(groupInfo->_chat_msgs);
    }
    result.historyBeforeSeq = dependencies.state->historyBeforeSeq;

    const int dialogUid = resolveGroupDialogUid(dependencies, result.groupId);
    clearUnread(dependencies, dialogUid, &result.unreadCleared);
    result.success = true;
    groupInfo = groupById(dependencies, result.groupId);
    result.cachedMessageCount = groupInfo ? static_cast<int>(groupInfo->_chat_msgs.size()) : result.cachedMessageCount;
    applyEffects();
    return result;
}

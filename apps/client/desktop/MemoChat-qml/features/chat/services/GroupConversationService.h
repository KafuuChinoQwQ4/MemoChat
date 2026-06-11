#ifndef GROUPCONVERSATIONSERVICE_H
#define GROUPCONVERSATIONSERVICE_H

#include "DialogListTypes.h"

#include <QByteArray>
#include <QHash>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <functional>
#include <memory>
#include <vector>

class ChatMessageModel;
class FriendListModel;
class GroupChatCacheStore;
struct GroupChatMsg;
struct GroupInfoData;
struct TextChatData;

struct GroupConversationState
{
    QHash<QString, qint64> pendingMsgGroupMap;
    qint64 historyBeforeSeq = 0;
    bool historyHasMore = true;

    void reset()
    {
        pendingMsgGroupMap.clear();
        historyBeforeSeq = 0;
        historyHasMore = true;
    }
};

struct GroupConversationContext
{
    int selfUid = 0;
    QString selfName;
    QString selfIcon;
    qint64 currentGroupId = 0;
    int currentPrivatePeerUid = 0;
};

struct GroupConversationDependencies
{
    GroupConversationState* state = nullptr;
    QMap<int, qint64>* dialogUidMap = nullptr;
    GroupChatCacheStore* cacheStore = nullptr;
    ChatMessageModel* messageModel = nullptr;
    FriendListModel* groupListModel = nullptr;
    FriendListModel* dialogListModel = nullptr;
    std::function<std::shared_ptr<GroupInfoData>(qint64)> groupById;
    std::function<void(std::shared_ptr<GroupInfoData>)> upsertGroup;
    std::function<void(qint64, const std::shared_ptr<TextChatData>&)> upsertGroupMessage;
    std::function<bool(qint64, const QString&, const QString&)> updateGroupMessageState;
    std::function<bool(qint64, const QString&, const QString&, const QString&, qint64, qint64)>
        updateGroupMessageContent;
    std::function<int(qint64, int, qint64)> markGroupOutgoingReadUntil;
    std::function<void(int, const QByteArray&)> dispatchPayload;
    std::function<void(FriendListModel&, int)> clearGroupUnreadAndMention;
    std::function<void(FriendListModel&, int, bool)> incrementGroupUnreadAndMention;
    std::function<DialogDecorationState()> dialogDecorationState;
};

struct GroupSendRequest
{
    GroupConversationContext context;
    qint64 groupId = 0;
    QString content;
    QString previewText;
    QString replyToMsgId;
};

struct GroupSendResult
{
    bool success = false;
    QString errorText;
    QString msgId;
    QByteArray compactPayload;
    std::shared_ptr<TextChatData> optimisticMessage;
    qint64 createdAt = 0;
    qint64 replyToServerMsgId = 0;
    bool dispatched = false;
    bool cacheUpdated = false;
    bool modelAppended = false;
    bool previewUpdated = false;
    bool unreadCleared = false;
};

struct GroupHistoryBuildRequest
{
    int selfUid = 0;
    qint64 groupId = 0;
    bool loading = false;
    int limit = 50;
    bool bootstrap = false;
};

struct GroupHistoryBuildResult
{
    bool success = false;
    bool skipped = false;
    QByteArray compactPayload;
    qint64 beforeSeq = 0;
    bool loadingSet = false;
    bool dispatched = false;
};

struct GroupHistoryRequestCommand
{
    int selfUid = 0;
    qint64 currentGroupId = 0;
    qint64 targetGroupId = 0;
    bool currentGroupLoading = false;
    int limit = 50;
    bool bootstrap = false;
};

struct GroupHistoryRequestPort
{
    std::function<void(bool)> setGroupHistoryLoading;
    std::function<void(bool)> setPrivateHistoryLoading;
};

struct GroupHistoryRequestResult
{
    bool success = false;
    bool skipped = false;
    bool skippedByLoading = false;
    bool skippedByNoMore = false;
    bool currentDialog = false;
    bool loadingProjected = false;
    bool dispatched = false;
    qint64 groupId = 0;
    qint64 currentGroupId = 0;
    qint64 beforeSeq = 0;
    QByteArray compactPayload;
};

struct GroupOpenRequest
{
    GroupConversationContext context;
    qint64 groupId = 0;
    int cacheLimit = 50;
    bool resetHistory = true;
};

struct GroupOpenResult
{
    bool success = false;
    qint64 groupId = 0;
    bool cacheLoaded = false;
    bool modelSet = false;
    bool modelCleared = false;
    qint64 requestedReadAckTs = 0;
};

struct GroupHistoryResponseRequest
{
    GroupConversationContext context;
    QJsonObject payload;
};

struct GroupHistoryResponseResult
{
    bool success = false;
    qint64 groupId = 0;
    qint64 currentGroupId = 0;
    bool currentDialog = false;
    bool cacheUpdated = false;
    bool modelSet = false;
    bool canLoadMore = false;
    qint64 historyBeforeSeq = 0;
    qint64 requestedReadAckTs = 0;
    bool unreadCleared = false;
    int messageCount = 0;
    int cachedMessageCount = 0;
    bool loadingCleared = false;
    bool canLoadMoreProjected = false;
    bool readAckRequested = false;
    bool statusSet = false;
};

struct GroupHistoryResponsePort
{
    std::function<void(bool)> setGroupHistoryLoading;
    std::function<void(bool)> setPrivateHistoryLoading;
    std::function<void(bool)> setCanLoadMorePrivateHistory;
    std::function<void(qint64, qint64)> sendGroupReadAck;
    std::function<void(const QString&, bool)> setGroupStatus;
};

struct GroupIncomingRequest
{
    GroupConversationContext context;
    std::shared_ptr<GroupChatMsg> message;
};

struct GroupIncomingResult
{
    bool success = false;
    qint64 groupId = 0;
    int dialogUid = 0;
    bool ensuredGroup = false;
    bool cacheUpdated = false;
    bool previewUpdated = false;
    bool mentionedMe = false;
    bool unreadCleared = false;
    bool unreadIncremented = false;
    bool modelAppended = false;
    qint64 requestedReadAckTs = 0;
};

struct GroupAckRequest
{
    GroupConversationContext context;
    int reqId = 0;
    int errorCode = 0;
    QJsonObject payload;
};

struct GroupAckResult
{
    bool success = false;
    qint64 groupId = 0;
    QString clientMsgId;
    QString nextState;
    QString statusText;
    bool statusIsError = false;
    bool correctedMessage = false;
    bool pendingRemoved = false;
    bool cacheUpdated = false;
    bool modelPatched = false;
    bool previewUpdated = false;
    qint64 requestedReadAckTs = 0;
};

struct GroupMutationRequest
{
    GroupConversationContext context;
    int reqId = 0;
    QJsonObject payload;
    QString event;
};

struct GroupMutationResult
{
    bool success = false;
    qint64 groupId = 0;
    QString msgId;
    QString statusText;
    bool statusIsError = false;
    bool userManagerUpdated = false;
    bool cacheUpdated = false;
    bool modelSet = false;
};

struct GroupReadAckRequest
{
    GroupConversationContext context;
    QJsonObject payload;
};

struct GroupReadAckResult
{
    bool success = false;
    qint64 groupId = 0;
    int readerUid = 0;
    qint64 readTs = 0;
    int updatedCount = 0;
    bool cacheUpdated = false;
    bool modelSet = false;
};

class GroupConversationService
{
public:
    static GroupSendResult sendText(const GroupSendRequest& request, const GroupConversationDependencies& dependencies);
    static GroupHistoryBuildResult buildHistoryRequest(const GroupHistoryBuildRequest& request,
                                                       const GroupConversationDependencies& dependencies);
    static GroupHistoryRequestResult requestHistory(const GroupHistoryRequestCommand& request,
                                                    const GroupConversationDependencies& dependencies,
                                                    const GroupHistoryRequestPort& port = {});
    static GroupOpenResult openGroupConversation(const GroupOpenRequest& request,
                                                 const GroupConversationDependencies& dependencies);
    static GroupHistoryResponseResult handleHistoryResponse(const GroupHistoryResponseRequest& request,
                                                            const GroupConversationDependencies& dependencies,
                                                            const GroupHistoryResponsePort& port = {});
    static GroupIncomingResult handleIncomingMessage(const GroupIncomingRequest& request,
                                                     const GroupConversationDependencies& dependencies);
    static GroupAckResult handleMessageAck(const GroupAckRequest& request,
                                           const GroupConversationDependencies& dependencies);
    static GroupAckResult handleMessageStatus(const GroupAckRequest& request,
                                              const GroupConversationDependencies& dependencies);
    static GroupAckResult handleMessageError(const GroupAckRequest& request,
                                             const GroupConversationDependencies& dependencies);
    static GroupMutationResult handleMessageMutation(const GroupMutationRequest& request,
                                                     const GroupConversationDependencies& dependencies);
    static GroupReadAckResult handleReadAckEvent(const GroupReadAckRequest& request,
                                                 const GroupConversationDependencies& dependencies);
    static QByteArray buildReadAckPayload(int selfUid, qint64 groupId, qint64 readTs);
};

#endif // GROUPCONVERSATIONSERVICE_H

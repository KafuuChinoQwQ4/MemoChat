#ifndef CHATFEATURECONTROLLER_H
#define CHATFEATURECONTROLLER_H

#include "ChatDialogListResponseService.h"
#include "ChatDialogSelectionService.h"
#include "ChatDialogRuntimeState.h"
#include "ChatMessageModel.h"
#include "ChatPendingSendQueueState.h"
#include "DialogListTypes.h"
#include "GroupChatCacheStore.h"
#include "GroupConversationService.h"
#include "IncomingMessageRouter.h"
#include "MessageMutationCommandService.h"
#include "PrivateChatCacheStore.h"
#include "PrivateChatHistoryService.h"
#include "PrivateChatSendService.h"
#include "UploadedAttachmentDispatchService.h"
#include "ChatViewModel.h"

#include <QByteArray>
#include <QMap>
#include <QObject>
#include <QString>
#include <functional>
#include <memory>
#include <vector>

struct OutgoingChatPacket;
class FriendListModel;
struct AuthInfo;
struct AuthRsp;
struct PrivateChatEventDependencies;
struct PrivateForwardResponseRequest;
struct PrivateForwardResponseResult;
struct PrivateIncomingMessageRequest;
struct PrivateIncomingMessageResult;
struct PrivateMessageChangedRequest;
struct PrivateMessageChangedResult;
struct PrivateMessageMutationResponseRequest;
struct PrivateMessageStatusRequest;
struct PrivateMessageStatusResult;
struct PrivateHistoryBootstrapDependencies;
struct PrivateHistoryBootstrapRequest;
struct PrivateHistoryLoadCurrentDependencies;
struct PrivateHistoryLoadCurrentRequest;
struct PrivateHistoryLoadCurrentResult;
struct PrivateHistoryLoadMoreDependencies;
struct PrivateHistoryLoadMoreRequest;
struct PrivateHistoryRequestBuildDependencies;
struct PrivateHistoryRequestBuildRequest;
struct PrivateHistoryRequestBuildResult;
struct PrivateHistoryResponseDependencies;
struct PrivateHistoryResponseRequest;
struct PrivateHistoryResponseResult;
struct PrivateReadAckRequest;
struct PrivateReadAckResult;
struct FriendInfo;
struct GroupInfoData;
struct TextChatData;
class UserMgr;

struct ChatDialogSnapshotRefreshResult
{
    int privateCount = 0;
    int groupCount = 0;
    int totalCount = 0;
};

struct ChatViewProjectionState
{
    int chatTab = 0;
    int currentDialogUid = 0;
    QString currentChatPeerName;
    QString currentChatPeerIcon;
    bool hasCurrentChat = false;
    bool hasCurrentGroup = false;
    int currentGroupRole = 0;
    QString currentDraftText;
    QVariantList currentPendingAttachments;
    bool currentDialogPinned = false;
    bool currentDialogMuted = false;
    bool hasPendingReply = false;
    QString replyTargetName;
    QString replyPreviewText;
    bool dialogsReady = false;
    bool chatLoadingMore = false;
    bool canLoadMoreChats = false;
    bool privateHistoryLoading = false;
    bool canLoadMorePrivateHistory = false;
    bool mediaUploadInProgress = false;
    QString mediaUploadProgressText;
};

struct ChatViewProjectionPort
{
    std::function<ChatViewProjectionState()> snapshot;
};

struct ChatListPagingSnapshot
{
    bool initialized = false;
    bool loading = false;
};

struct ChatListPagingResult
{
    bool success = false;
    bool skipped = false;
    bool initialized = false;
    bool pageAppended = false;
    bool loadingProjected = false;
    bool canLoadMoreProjected = false;
    bool incomingFlushed = false;
    int itemCount = 0;
};

struct ChatListPagingPort
{
    std::function<ChatListPagingSnapshot()> snapshot;
    std::function<std::vector<std::shared_ptr<FriendInfo>>()> nextPage;
    std::function<bool()> loadFinished;
    std::function<void()> markPageLoaded;
    std::function<void(bool)> setInitialized;
    std::function<void(bool)> setLoading;
    std::function<void(bool)> setCanLoadMore;
    std::function<void()> flushIncomingMessages;
};

struct ChatDialogNavigationPort
{
    std::function<void()> ensureChatListInitialized;
    std::function<void(int)> selectDialogByUid;
    std::function<void(int)> selectChatIndex;
    std::function<void()> loadMoreChats;
    std::function<void()> loadMorePrivateHistory;
};

struct ChatGroupConversationPort
{
    std::function<void()> ensureGroupsInitialized;
    std::function<void(int)> selectGroupIndex;
};

struct ChatPrivateCommandPort
{
    std::function<void(const QString&)> sendCurrentComposerPayload;
    std::function<void()> sendImageMessage;
    std::function<void()> sendFileMessage;
};

struct ChatCurrentSendSnapshot
{
    int selfUid = 0;
    int currentPrivatePeerUid = 0;
    GroupConversationContext context;
    qint64 currentGroupId = 0;
};

struct ChatCurrentSendPort
{
    std::function<ChatCurrentSendSnapshot()> snapshot;
    std::function<PrivateChatSendDependencies()> privateDependencies;
    std::function<GroupConversationDependencies()> groupDependencies;
};

struct ChatContentDispatchPort
{
    std::function<bool()> isTransportReady;
    std::function<void(const QString&, bool)> setTip;
    std::function<int()> currentPrivatePeerUid;
    std::function<qint64()> currentGroupId;
    std::function<std::shared_ptr<FriendInfo>(int)> privateFriendById;
    std::function<std::shared_ptr<GroupInfoData>(qint64)> groupById;
};

struct ChatCurrentGroupHistorySnapshot
{
    int selfUid = 0;
    qint64 currentGroupId = 0;
    bool groupHistoryLoading = false;
};

struct ChatCurrentGroupHistoryPort
{
    std::function<ChatCurrentGroupHistorySnapshot()> snapshot;
    std::function<GroupConversationDependencies()> dependencies;
    GroupHistoryRequestPort requestPort;
};

struct ChatGroupConversationClearRequest
{
    qint64 groupId = 0;
    qint64 currentGroupId = 0;
};

struct ChatGroupConversationClearResult
{
    bool success = false;
    qint64 targetGroupId = 0;
    int dialogUid = 0;
    bool dialogRemoved = false;
    bool groupRemoved = false;
    bool mappingRemoved = false;
    bool currentConversationCleared = false;
    bool currentRuntimeReset = false;
};

struct ChatGroupConversationClearPort
{
    std::function<int(qint64)> dialogUidForGroup;
    std::function<void(int)> removeGroupByDialogUid;
    std::function<void(int)> removeGroupDialogMapping;
    std::function<void()> clearCurrentGroup;
    std::function<void(bool)> setGroupHistoryLoading;
    std::function<void(bool)> setCanLoadMorePrivateHistory;
    std::function<void()> clearMessageModel;
    std::function<void()> resetCurrentPeer;
    std::function<void(const QString&, const QString&, const QString&)> setPendingReplyContext;
    std::function<void(const QString&)> setCurrentDraftText;
    std::function<void(bool)> setCurrentDialogPinned;
    std::function<void(bool)> setCurrentDialogMuted;
    std::function<void()> emitCurrentDialogUidChangedIfNeeded;
};

struct ChatUploadedAttachmentDispatchPort
{
    std::function<bool(const QString&, const QString&)> dispatchCurrentPrivateContent;
    std::function<bool(const QString&, const QString&)> dispatchCurrentGroupContent;
    std::function<bool(const OutgoingChatPacket&, QString*)> dispatchPrivatePacket;
    std::function<void(int, const QByteArray&)> dispatchGroupPayload;
};

struct ChatPendingAttachmentPort
{
    std::function<void(const QString&)> removePendingAttachment;
    std::function<void()> clearPendingAttachments;
};

struct ChatDialogRuntimePort
{
    std::function<void(const QString&)> syncCurrentDraftText;
    std::function<void(bool)> syncCurrentDialogPinned;
    std::function<void(bool)> syncCurrentDialogMuted;
};

struct ChatShellIntentPort
{
    std::function<void()> startVoiceChat;
    std::function<void()> startVideoChat;
};

struct ChatMessageMutationSnapshot
{
    int selfUid = 0;
    MessageMutationTarget target;
};

struct ChatMessageMutationPort
{
    std::function<ChatMessageMutationSnapshot()> snapshot;
    std::function<bool(const QString&)> privateMessageExists;
    std::function<bool(const QString&)> canRevokeMessage;
    std::function<void(const QString&, bool)> setPrivateStatus;
    std::function<void(const QString&, bool)> setGroupStatus;
    std::function<void(int, const QByteArray&)> dispatchPayload;
};

struct ChatHistoryLoadMoreRequest
{
    int selfUid = 0;
    int currentPrivatePeerUid = 0;
    qint64 currentGroupId = 0;
    bool privateHistoryLoading = false;
    bool canLoadMorePrivateHistory = false;
    bool groupHistoryLoading = false;
    int groupLimit = 50;
};

struct ChatHistoryLoadMoreResult
{
    bool success = false;
    bool skipped = false;
    bool groupPath = false;
    bool privatePath = false;
    bool dispatched = false;
    bool loadingSet = false;
    bool groupLoadingProjected = false;
    QString errorText;
    int peerUid = 0;
    qint64 groupId = 0;
    qint64 beforeTs = 0;
    QString beforeMsgId;
};

struct ChatCurrentHistoryPort
{
    std::function<ChatHistoryLoadMoreRequest()> snapshot;
    std::function<PrivateHistoryLoadMoreDependencies()> privateDependencies;
    std::function<GroupConversationDependencies()> groupDependencies;
    GroupHistoryRequestPort groupRequestPort;
};

class ChatFeatureController : public QObject
{
    Q_OBJECT

public:
    explicit ChatFeatureController(ChatViewModel& viewModel, QObject* parent = nullptr);

    void bindRequests();

    FriendListModel& chatListModel();
    const FriendListModel& chatListModel() const;
    FriendListModel& dialogListModel();
    const FriendListModel& dialogListModel() const;
    ChatMessageModel& messageModel();
    const ChatMessageModel& messageModel() const;
    PrivateChatCacheStore& privateCacheStore();
    GroupChatCacheStore& groupCacheStore();
    void setViewProjectionPort(ChatViewProjectionPort port);
    void syncViewModelState();
    void syncViewModelState(const ChatViewProjectionState& state);
    void clearMessageModel();
    int messageCount() const;
    bool containsMessage(const QString& msgId) const;
    bool canRevokeMessage(const QString& msgId) const;
    void setMessageDownloadAuthContext(int uid, const QString& token);
    void openCacheStoresForUser(int uid);
    void closeCacheStores();
    void clearConversationLists();
    void resetForPostLoginEntry();
    void resetModelsForLogout();
    void resetRuntimeForLogout();
    QMap<int, qint64>& groupDialogUidMap();
    const QMap<int, qint64>& groupDialogUidMap() const;
    QMap<int, qint64>* groupDialogUidMapPtr();
    void resetGroupDialogIdentityMap();
    int resolveGroupDialogUid(qint64 groupId);
    qint64 groupIdForDialogUid(int dialogUid) const;
    void removeGroupDialogUid(int dialogUid);
    ChatDialogSnapshotRefreshResult
    replaceDialogListFromSnapshots(const std::vector<std::shared_ptr<FriendInfo>>& friends,
                                   const std::vector<std::shared_ptr<GroupInfoData>>& groups);
    ChatDialogSnapshotRefreshResult
    upsertDialogListFromSnapshots(const std::vector<std::shared_ptr<FriendInfo>>& friends,
                                  const std::vector<std::shared_ptr<GroupInfoData>>& groups);
    ChatListPagingResult ensureChatListInitialized();
    ChatListPagingResult loadMoreChats();

    void setChatListPagingPort(ChatListPagingPort port);
    void setDialogNavigationPort(ChatDialogNavigationPort port);
    void setGroupConversationPort(ChatGroupConversationPort port);
    void setDialogSelectionPort(ChatDialogSelectionPort port);
    void setPrivateCommandPort(ChatPrivateCommandPort port);
    void setCurrentSendPort(ChatCurrentSendPort port);
    void setContentDispatchPort(ChatContentDispatchPort port);
    void setCurrentHistoryPort(ChatCurrentHistoryPort port);
    void setCurrentGroupHistoryPort(ChatCurrentGroupHistoryPort port);
    void setUploadedAttachmentDispatchPort(ChatUploadedAttachmentDispatchPort port);
    void setPendingAttachmentPort(ChatPendingAttachmentPort port);
    void setDialogRuntimePort(ChatDialogRuntimePort port);
    void setDialogCommandPort(ChatDialogCommandPort port);
    void setShellIntentPort(ChatShellIntentPort port);
    void setReadAckPort(ChatReadAckPort port);
    void setMessageMutationPort(ChatMessageMutationPort port);
    void setPrivatePacketDispatcher(std::function<bool(const OutgoingChatPacket&, QString*)> dispatcher);
    void setPrivateAppendFriendMessages(
        std::function<void(int, const std::vector<std::shared_ptr<TextChatData>>&)> appendFriendMessages);
    void bindPrivateMessageStore(std::shared_ptr<UserMgr> userMgr);
    PrivateChatSendDependencies privateSendDependencies();

    PrivateHistoryRequestState& privateHistoryState();
    const PrivateHistoryRequestState& privateHistoryState() const;
    void clearPrivateHistoryState();
    GroupConversationState& groupConversationState();
    const GroupConversationState& groupConversationState() const;
    void clearGroupConversationState();
    ChatGroupConversationClearResult clearGroupConversation(const ChatGroupConversationClearRequest& request,
                                                            const ChatGroupConversationClearPort& port);
    void resetGroupHistoryState();
    void clearDialogRuntimeState();
    ChatPrivateConversationClearResult
    clearPrivateConversationIfSelected(const ChatPrivateConversationClearRequest& request,
                                       const ChatPrivateConversationClearPort& port);
    void upsertContactFromAuthInfo(const std::shared_ptr<AuthInfo>& authInfo);
    void upsertContactFromAuthRsp(const std::shared_ptr<AuthRsp>& authRsp);
    void upsertContactFriendInfo(const std::shared_ptr<FriendInfo>& friendInfo);
    ChatPrivateConversationClearResult removeContactConversation(int friendUid,
                                                                 const ChatPrivateConversationClearPort& port);
    static QString dialogMetaActionText(int reqId);
    QString currentDraftText() const;
    QVariantList currentPendingAttachments() const;
    bool hasPendingAttachments() const;
    bool currentDialogPinned() const;
    bool currentDialogMuted() const;
    bool hasPendingReply() const;
    QString replyTargetName() const;
    QString replyPreviewText() const;
    QString replyToMsgId() const;
    bool setCurrentDraftText(const QString& text);
    bool setCurrentPendingAttachments(const QVariantList& attachments);
    bool setCurrentDialogPinned(bool pinned);
    bool setCurrentDialogMuted(bool muted);
    bool setPendingReplyContext(const QString& msgId, const QString& senderName, const QString& previewText);
    bool updateLastEmittedDialogUid(int dialogUid);
    ChatDialogRuntimeSnapshot snapshotForDialog(int dialogUid) const;
    DialogDecorationState dialogDecorationState() const;
    DialogDecorationState dialogDecorationStateWithoutServerMeta() const;
    void resetServerDialogMeta();
    void seedServerDialogMeta(int dialogUid, int pinnedRank, int muteState);
    void removeMentionForDialog(int dialogUid);
    void setMentionCountForDialog(int dialogUid, int count);
    void clearGroupUnreadAndMention(FriendListModel& dialogListModel, int dialogUid);
    void incrementGroupUnreadAndMention(FriendListModel& dialogListModel, int dialogUid, bool mentioned);
    void clearMentionState();
    void clearPendingAttachmentStore();
    void clearDialogDecorationStore();
    void removeDialogDecoration(int dialogUid);
    void loadPersistentDialogStore(int ownerUid);
    void savePersistentDialogStore(int ownerUid) const;
    QString draftForDialog(int dialogUid) const;
    int muteStateForDialog(int dialogUid) const;
    QVariantList pendingAttachmentsForDialog(int dialogUid) const;
    void setPendingAttachmentsForDialog(int dialogUid, const QVariantList& attachments);
    void clearPendingAttachmentsForDialog(int dialogUid);
    ChatPendingAttachmentCommandResult addPendingAttachmentsForDialog(int dialogUid, const QVariantList& attachments);
    ChatPendingAttachmentCommandResult removePendingAttachmentByIdForDialog(int dialogUid, const QString& attachmentId);
    bool beginPendingAttachmentSend(int dialogUid, int chatUid, qint64 groupId, const QVariantList& attachments);
    bool beginSinglePendingAttachmentSend(int dialogUid, int chatUid, qint64 groupId, const QVariantMap& attachment);
    bool pendingAttachmentSendQueueEmpty() const;
    ChatPendingSendQueueSnapshot pendingAttachmentSendSnapshot() const;
    QVariantMap currentPendingAttachmentToSend() const;
    int pendingAttachmentSendDialogUid() const;
    int pendingAttachmentSendChatUid() const;
    qint64 pendingAttachmentSendGroupId() const;
    int pendingAttachmentSendTotalCount() const;
    int pendingAttachmentSendRemainingCount() const;
    int pendingAttachmentSendCurrentIndex() const;
    bool advancePendingAttachmentSendQueue();
    void resetPendingAttachmentSendQueue();
    UploadedAttachmentDispatchResult dispatchUploadedAttachment(const QVariantMap& attachment,
                                                                const UploadedMediaInfo& uploaded,
                                                                const UploadedAttachmentDestination& destination);
    ChatDialogDraftResult updateDraftForDialog(int dialogUid, const QString& text);
    ChatDialogDraftResult clearDraftForDialog(int dialogUid);
    ChatDialogPinnedResult togglePinnedForDialog(int dialogUid);
    ChatDialogMutedResult toggleMutedForDialog(int dialogUid);
    ChatDialogCommandResult updateDialogDraft(const ChatDialogCommandRequest& request,
                                              ChatDialogCommandDependencies dependencies);
    ChatDialogCommandResult updateCurrentDraft(const QString& text);
    ChatDialogCommandResult clearDialogDraft(const ChatDialogCommandRequest& request,
                                             ChatDialogCommandDependencies dependencies);
    ChatDialogCommandResult clearDialogDraftByUid(int dialogUid);
    ChatDialogCommandResult toggleDialogPinned(const ChatDialogCommandRequest& request,
                                               ChatDialogCommandDependencies dependencies);
    ChatDialogCommandResult toggleCurrentDialogPinned();
    ChatDialogCommandResult toggleDialogPinnedByUid(int dialogUid);
    ChatDialogCommandResult toggleDialogMuted(const ChatDialogCommandRequest& request,
                                              ChatDialogCommandDependencies dependencies);
    ChatDialogCommandResult toggleCurrentDialogMuted();
    ChatDialogCommandResult toggleDialogMutedByUid(int dialogUid);
    ChatMarkDialogReadResult markDialogRead(const ChatMarkDialogReadRequest& request,
                                            ChatMarkDialogReadDependencies dependencies);
    ChatMarkDialogReadResult markDialogReadByUid(int dialogUid);
    ChatDialogMetaResponseResult handleDialogMetaResponse(const ChatDialogMetaResponseRequest& request,
                                                          ChatDialogMetaResponseDependencies dependencies);
    ChatDialogDraftResult applyRemoteDraftMeta(int dialogUid, const QString& draftText, int muteState);
    ChatDialogPinnedResult applyRemotePinnedMeta(int dialogUid, int pinnedRank);
    static QByteArray buildDraftSyncPayload(int selfUid,
                                            const QString& dialogType,
                                            int peerUid,
                                            qint64 groupId,
                                            const QString& draftText,
                                            int muteState = -1);
    static QByteArray
    buildPinDialogPayload(int selfUid, const QString& dialogType, int peerUid, qint64 groupId, int pinnedRank);

    PrivateChatSendResult sendPrivateText(const PrivateChatSendRequest& request,
                                          PrivateChatSendDependencies dependencies);
    PrivateChatSendResult sendCurrentPrivateText(const QString& content, const QString& previewText);
    bool dispatchCurrentPrivateContent(const QString& content, const QString& previewText);
    MessageMutationCommandResult runCurrentMessageMutation(MessageMutationCommand command,
                                                           const QString& msgId,
                                                           const QString& text = QString()) const;
    MessageMutationCommandResult editCurrentMessage(const QString& msgId, const QString& text) const;
    MessageMutationCommandResult revokeCurrentMessage(const QString& msgId) const;
    MessageMutationCommandResult forwardCurrentMessage(const QString& msgId) const;
    PrivateHistoryLoadCurrentResult loadCurrentPrivateHistory(const PrivateHistoryLoadCurrentRequest& request,
                                                              PrivateHistoryLoadCurrentDependencies dependencies);
    PrivateHistoryLoadCurrentDependencies
    privateHistoryLoadCurrentDependencies(std::shared_ptr<UserMgr> userMgr,
                                          std::function<void(const QString&)> setCurrentPeerName,
                                          std::function<void(const QString&)> setCurrentPeerIcon,
                                          std::function<void(bool)> setLoading,
                                          std::function<void(bool)> setCanLoadMore,
                                          std::function<void(int, qint64)> requestReadAck);
    PrivateHistoryRequestBuildResult buildPrivateHistoryRequest(const PrivateHistoryRequestBuildRequest& request,
                                                                PrivateHistoryRequestBuildDependencies dependencies);
    PrivateHistoryRequestBuildResult
    buildPrivateHistoryBootstrapRequest(const PrivateHistoryBootstrapRequest& request,
                                        const PrivateHistoryBootstrapDependencies& dependencies) const;
    PrivateHistoryRequestBuildResult
    buildPrivateHistoryLoadMoreRequest(const PrivateHistoryLoadMoreRequest& request,
                                       PrivateHistoryLoadMoreDependencies dependencies);
    ChatHistoryLoadMoreResult loadMoreCurrentHistory();
    ChatHistoryLoadMoreResult loadMoreCurrentHistory(const ChatHistoryLoadMoreRequest& request,
                                                     PrivateHistoryLoadMoreDependencies privateDependencies,
                                                     GroupConversationDependencies groupDependencies,
                                                     const GroupHistoryRequestPort& groupPort = {});
    PrivateHistoryResponseResult handlePrivateHistoryResponse(const PrivateHistoryResponseRequest& request,
                                                              PrivateHistoryResponseDependencies dependencies);
    PrivateHistoryResponseDependencies
    privateHistoryResponseDependencies(std::shared_ptr<UserMgr> userMgr,
                                       std::function<void(bool)> setLoading,
                                       std::function<void(bool)> setCanLoadMore,
                                       std::function<void(int, qint64)> requestReadAck);
    PrivateChatEventDependencies privateEventDependencies(std::shared_ptr<UserMgr> userMgr,
                                                          std::function<void(std::shared_ptr<AuthInfo>)> upsertContact,
                                                          std::function<void(int, qint64)> requestReadAck);
    PrivateIncomingMessageResult handlePrivateIncomingMessage(const PrivateIncomingMessageRequest& request,
                                                              PrivateChatEventDependencies dependencies);
    PrivateMessageChangedResult handlePrivateMessageChanged(const PrivateMessageChangedRequest& request,
                                                            PrivateChatEventDependencies dependencies);
    PrivateMessageChangedResult
    handlePrivateMessageMutationResponse(const PrivateMessageMutationResponseRequest& request,
                                         PrivateChatEventDependencies dependencies);
    PrivateForwardResponseResult handlePrivateForwardResponse(const PrivateForwardResponseRequest& request,
                                                              PrivateChatEventDependencies dependencies);
    PrivateReadAckResult handlePrivateReadAck(const PrivateReadAckRequest& request,
                                              PrivateChatEventDependencies dependencies);
    ChatReadAckCommandResult sendPrivateReadAck(const ChatReadAckCommand& request,
                                                const ChatReadAckDispatchPort& port = {}) const;
    ChatReadAckCommandResult sendPrivateReadAckForPeer(int peerUid, qint64 readTs = 0) const;
    PrivateMessageStatusResult handlePrivateMessageStatus(const PrivateMessageStatusRequest& request,
                                                          PrivateChatEventDependencies dependencies);
    GroupSendResult sendGroupText(const GroupSendRequest& request, GroupConversationDependencies dependencies);
    GroupConversationDependencies
    groupConversationDependencies(std::shared_ptr<UserMgr> userMgr,
                                  FriendListModel* groupListModel,
                                  std::function<void(int, const QByteArray&)> dispatchPayload);
    GroupSendResult sendCurrentGroupText(const QString& content, const QString& previewText);
    bool dispatchCurrentGroupContent(const QString& content, const QString& previewText);
    GroupHistoryBuildResult buildGroupHistoryRequest(const GroupHistoryBuildRequest& request,
                                                     GroupConversationDependencies dependencies);
    GroupHistoryRequestResult requestGroupHistory(const GroupHistoryRequestCommand& request,
                                                  GroupConversationDependencies dependencies,
                                                  const GroupHistoryRequestPort& port = {});
    GroupHistoryRequestResult requestCurrentGroupHistory();
    GroupHistoryRequestResult requestGroupHistoryForBootstrap(const GroupHistoryRequestCommand& request,
                                                              GroupConversationDependencies dependencies,
                                                              const GroupHistoryRequestPort& port = {});
    GroupOpenResult openGroupConversation(const GroupOpenRequest& request, GroupConversationDependencies dependencies);
    GroupHistoryResponseResult handleGroupHistoryResponse(const GroupHistoryResponseRequest& request,
                                                          GroupConversationDependencies dependencies,
                                                          const GroupHistoryResponsePort& port = {});
    GroupIncomingResult handleGroupIncomingMessage(const GroupIncomingRequest& request,
                                                   GroupConversationDependencies dependencies);
    GroupAckResult handleGroupMessageAck(const GroupAckRequest& request, GroupConversationDependencies dependencies);
    GroupAckResult handleGroupMessageStatus(const GroupAckRequest& request, GroupConversationDependencies dependencies);
    GroupAckResult handleGroupMessageError(const GroupAckRequest& request, GroupConversationDependencies dependencies);
    GroupMutationResult handleGroupMessageMutation(const GroupMutationRequest& request,
                                                   GroupConversationDependencies dependencies);
    GroupReadAckResult handleGroupReadAckEvent(const GroupReadAckRequest& request,
                                               GroupConversationDependencies dependencies);
    ChatReadAckCommandResult sendGroupReadAck(const ChatReadAckCommand& request,
                                              const ChatReadAckDispatchPort& port = {}) const;
    ChatReadAckCommandResult sendGroupReadAckForGroup(qint64 groupId, qint64 readTs = 0) const;
    ChatDialogListResponseEffect handleDialogListResponse(const QJsonObject& payload,
                                                          ChatDialogListResponseContext context,
                                                          const ChatDialogListAppPort& appPort);
    IncomingMessageRouteResult routePrivateIncomingMessage(const IncomingMessageRouterReadiness& readiness,
                                                           std::shared_ptr<TextChatMsg> message,
                                                           const IncomingMessageRouterDispatch& dispatch);
    IncomingMessageRouteResult routeGroupIncomingMessage(const IncomingMessageRouterReadiness& readiness,
                                                         std::shared_ptr<GroupChatMsg> message,
                                                         const IncomingMessageRouterDispatch& dispatch);
    int flushIncomingMessages(const IncomingMessageRouterReadiness& readiness,
                              const IncomingMessageRouterDispatch& dispatch);
    void clearIncomingMessages();
    int pendingIncomingMessageCount() const;
    void selectPrivateByIndex(int index);
    void selectPrivateByUid(int uid);
    void selectGroupByIndex(int index);
    void selectGroupByDialogUid(int dialogUid, qint64 groupId);
    void selectDialogByUid(int dialogUid);

private:
    ChatViewModel& _viewModel;
    bool _requestsBound = false;
    ChatViewProjectionPort _viewProjectionPort;
    ChatListPagingPort _chatListPagingPort;
    ChatDialogNavigationPort _dialogNavigationPort;
    ChatGroupConversationPort _groupConversationPort;
    ChatDialogSelectionPort _dialogSelectionPort;
    ChatPrivateCommandPort _privateCommandPort;
    ChatCurrentSendPort _currentSendPort;
    ChatContentDispatchPort _contentDispatchPort;
    ChatCurrentHistoryPort _currentHistoryPort;
    ChatCurrentGroupHistoryPort _currentGroupHistoryPort;
    ChatUploadedAttachmentDispatchPort _uploadedAttachmentDispatchPort;
    ChatPendingAttachmentPort _pendingAttachmentPort;
    ChatDialogRuntimePort _dialogRuntimePort;
    ChatDialogCommandPort _dialogCommandPort;
    ChatShellIntentPort _shellIntentPort;
    ChatReadAckPort _readAckPort;
    ChatMessageMutationPort _messageMutationPort;
    ChatDialogRuntimeState _dialogRuntimeState;
    FriendListModel _chatListModel;
    FriendListModel _dialogListModel;
    ChatMessageModel _messageModel;
    PrivateChatCacheStore _privateCacheStore;
    GroupChatCacheStore _groupCacheStore;
    QMap<int, qint64> _groupDialogUidMap;
    ChatPendingSendQueueState _pendingSendQueueState;
    PrivateHistoryRequestState _privateHistoryState;
    GroupConversationState _groupConversationState;
    IncomingMessageRouter _incomingMessageRouter;
    std::function<bool(const OutgoingChatPacket&, QString*)> _privatePacketDispatcher;
    std::function<void(int, const std::vector<std::shared_ptr<TextChatData>>&)> _privateAppendFriendMessages;
};

#endif // CHATFEATURECONTROLLER_H

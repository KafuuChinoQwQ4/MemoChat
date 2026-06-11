#include "AppChatDispatcherRouterFactory.h"

#include "AppChatDispatcherEventRouter.h"
#include "ChatEventDependenciesFactory.h"
#include "ChatFeatureController.h"
#include "ContactController.h"
#include "PrivateHistoryDependenciesFactory.h"

#include <utility>

namespace memochat::app
{

std::unique_ptr<AppChatDispatcherEventRouter> makeAppChatDispatcherEventRouter(
    AppSessionCoordinator& sessionCoordinator,
    AppChatConnectionCoordinator& connectionCoordinator,
    CallCoordinator& callCoordinator,
    ChatFeatureController& chatController,
    std::shared_ptr<UserMgr> userMgr,
    ContactController& contactController,
    GroupController& groupController,
    std::function<ContactEventActions()> makeContactEventActions,
    std::function<IncomingMessageRouterReadinessInputs()> makeIncomingMessageReadinessInputs,
    std::function<IncomingMessageRouterDispatchActions()> makeIncomingMessageDispatchActions,
    std::function<GroupConversationContext(int)> makeGroupConversationContext,
    FriendListModel* groupListModel,
    std::function<void(int, const QByteArray&)> dispatchPayload,
    std::function<memochat::group::GroupManagementEventContext()> makeGroupManagementEventContext,
    std::function<GroupManagementEventEffectActions()> makeGroupManagementEventEffectActions,
    AppChatDispatcherGroupResponseHandlers groupResponseHandlers,
    std::function<ChatDialogListResponseContext()> makeDialogListResponseContext,
    std::function<ChatDialogListAppPort()> makeDialogListAppPort,
    std::function<PrivateChatEventContext(int)> makePrivateChatEventContext,
    std::function<int()> currentPrivatePeerUid,
    std::function<void(bool)> setPrivateHistoryLoading,
    std::function<void(bool)> setCanLoadMorePrivateHistory,
    QObject* parent)
{
    return std::make_unique<AppChatDispatcherEventRouter>(
        sessionCoordinator,
        connectionCoordinator,
        callCoordinator,
        chatController,
        userMgr,
        contactController,
        groupController,
        [makeContactEventActions = std::move(makeContactEventActions)]()
        {
            return makeContactEventDependencies(makeContactEventActions());
        },
        [makeIncomingMessageReadinessInputs = std::move(makeIncomingMessageReadinessInputs)]()
        {
            return makeIncomingMessageRouterReadiness(makeIncomingMessageReadinessInputs());
        },
        [makeIncomingMessageDispatchActions = std::move(makeIncomingMessageDispatchActions)]()
        {
            return makeIncomingMessageRouterDispatch(makeIncomingMessageDispatchActions());
        },
        std::move(makeGroupConversationContext),
        [&chatController, userMgr, groupListModel, dispatchPayload = std::move(dispatchPayload)]()
        {
            return makeGroupConversationDependencies(chatController, userMgr, groupListModel, dispatchPayload);
        },
        std::move(makeGroupManagementEventContext),
        std::move(makeGroupManagementEventEffectActions),
        std::move(groupResponseHandlers),
        std::move(makeDialogListResponseContext),
        std::move(makeDialogListAppPort),
        std::move(makePrivateChatEventContext),
        [&chatController, userMgr, &contactController]()
        {
            return makePrivateChatEventDependencies(
                chatController,
                userMgr,
                [&contactController](std::shared_ptr<AuthInfo> authInfo)
                {
                    contactController.upsertContact(std::move(authInfo));
                },
                [&chatController](int peerUid, qint64 readTs)
                {
                    chatController.sendPrivateReadAckForPeer(peerUid, readTs);
                });
        },
        std::move(currentPrivatePeerUid),
        [&chatController,
         userMgr,
         setPrivateHistoryLoading = std::move(setPrivateHistoryLoading),
         setCanLoadMorePrivateHistory = std::move(setCanLoadMorePrivateHistory)]()
        {
            return makePrivateHistoryResponseDependencies(chatController,
                                                          userMgr,
                                                          setPrivateHistoryLoading,
                                                          setCanLoadMorePrivateHistory,
                                                          [&chatController](int peerUid, qint64 readTs)
                                                          {
                                                              chatController.sendPrivateReadAckForPeer(peerUid, readTs);
                                                          });
        },
        parent);
}

} // namespace memochat::app

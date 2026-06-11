#pragma once

#include "AppChatDispatcherGroupResponseHandlers.h"
#include "ChatDialogListResponseService.h"
#include "ContactEventDependenciesFactory.h"
#include "GroupConversationService.h"
#include "GroupManagementEffectPortFactory.h"
#include "GroupManagementEventService.h"
#include "IncomingMessageRouterFactory.h"
#include "PrivateChatEventService.h"

#include <QByteArray>
#include <functional>
#include <memory>

class AppChatConnectionCoordinator;
class AppChatDispatcherEventRouter;
class AppSessionCoordinator;
class CallCoordinator;
class ChatFeatureController;
class ContactController;
class FriendListModel;
class GroupController;
class QObject;
class UserMgr;

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
    QObject* parent);

} // namespace memochat::app

#include "AppChatDispatcherEventRouter.h"

#include "AppChatConnectionCoordinator.h"
#include "AppCoordinators.h"
#include "ChatFeatureController.h"
#include "ChatProtocolRouter.h"
#include "ContactController.h"
#include "GroupController.h"
#include "usermgr.h"

#include <QDebug>
#include <utility>

AppChatDispatcherEventRouter::AppChatDispatcherEventRouter(
    AppSessionCoordinator& sessionCoordinator,
    AppChatConnectionCoordinator& connectionCoordinator,
    CallCoordinator& callCoordinator,
    ChatFeatureController& chatController,
    std::shared_ptr<UserMgr> userMgr,
    ContactController& contactController,
    GroupController& groupController,
    std::function<ContactEventDependencies()> makeContactEventDependencies,
    std::function<IncomingMessageRouterReadiness()> makeIncomingMessageReadiness,
    std::function<IncomingMessageRouterDispatch()> makeIncomingMessageDispatch,
    std::function<GroupConversationContext(int)> makeGroupConversationContext,
    std::function<GroupConversationDependencies()> makeGroupConversationDependencies,
    std::function<memochat::group::GroupManagementEventContext()> makeGroupManagementEventContext,
    std::function<memochat::app::GroupManagementEventEffectActions()> makeGroupManagementEventEffectActions,
    memochat::app::AppChatDispatcherGroupResponseHandlers groupResponseHandlers,
    std::function<ChatDialogListResponseContext()> makeDialogListResponseContext,
    std::function<ChatDialogListAppPort()> makeDialogListAppPort,
    std::function<PrivateChatEventContext(int)> makePrivateChatEventContext,
    std::function<PrivateChatEventDependencies()> makePrivateChatEventDependencies,
    std::function<int()> currentPrivatePeerUid,
    std::function<PrivateHistoryResponseDependencies()> makePrivateHistoryResponseDependencies,
    QObject* parent)
    : QObject(parent)
    , _session_coordinator(sessionCoordinator)
    , _connection_coordinator(connectionCoordinator)
    , _call_coordinator(callCoordinator)
    , _chat_controller(chatController)
    , _user_mgr(std::move(userMgr))
    , _contact_controller(contactController)
    , _group_controller(groupController)
    , _make_contact_event_dependencies(std::move(makeContactEventDependencies))
    , _make_incoming_message_readiness(std::move(makeIncomingMessageReadiness))
    , _make_incoming_message_dispatch(std::move(makeIncomingMessageDispatch))
    , _make_group_conversation_context(std::move(makeGroupConversationContext))
    , _make_group_conversation_dependencies(std::move(makeGroupConversationDependencies))
    , _make_group_management_event_context(std::move(makeGroupManagementEventContext))
    , _make_group_management_event_effect_actions(std::move(makeGroupManagementEventEffectActions))
    , _group_response_handlers(std::move(groupResponseHandlers))
    , _make_dialog_list_response_context(std::move(makeDialogListResponseContext))
    , _make_dialog_list_app_port(std::move(makeDialogListAppPort))
    , _make_private_chat_event_context(std::move(makePrivateChatEventContext))
    , _make_private_chat_event_dependencies(std::move(makePrivateChatEventDependencies))
    , _current_private_peer_uid(std::move(currentPrivatePeerUid))
    , _make_private_history_response_dependencies(std::move(makePrivateHistoryResponseDependencies))
{
}

void AppChatDispatcherEventRouter::onChatLoginFailed(int err)
{
    _connection_coordinator.onChatLoginFailed(err);
}

void AppChatDispatcherEventRouter::onSwitchToChat()
{
    _session_coordinator.onSwitchToChat();
}

void AppChatDispatcherEventRouter::onTextChatMsg(std::shared_ptr<TextChatMsg> msg)
{
    const IncomingMessageRouteResult result =
        _chat_controller.routePrivateIncomingMessage(_make_incoming_message_readiness(),
                                                     std::move(msg),
                                                     _make_incoming_message_dispatch());
    if (result.buffered || result.applied)
    {
        return;
    }
}

void AppChatDispatcherEventRouter::onAddAuthFriend(std::shared_ptr<AuthInfo> authInfo)
{
    ContactEventService::handleAddAuthFriend(_contact_controller,
                                             std::move(authInfo),
                                             _make_contact_event_dependencies());
}

void AppChatDispatcherEventRouter::onDeleteFriendRsp(int error, int friendUid)
{
    ContactEventService::handleDeleteFriendRsp(_contact_controller,
                                               error,
                                               friendUid,
                                               _make_contact_event_dependencies());
}

void AppChatDispatcherEventRouter::onAuthRsp(std::shared_ptr<AuthRsp> authRsp)
{
    ContactEventService::handleAuthRsp(_contact_controller, std::move(authRsp), _make_contact_event_dependencies());
}

void AppChatDispatcherEventRouter::onUserSearch(std::shared_ptr<SearchInfo> searchInfo)
{
    ContactEventService::handleUserSearch(_contact_controller,
                                          std::move(searchInfo),
                                          _make_contact_event_dependencies());
}

void AppChatDispatcherEventRouter::onFriendApply(std::shared_ptr<AddFriendApply> applyInfo)
{
    ContactEventService::handleFriendApply(_contact_controller,
                                           std::move(applyInfo),
                                           _make_contact_event_dependencies());
}

void AppChatDispatcherEventRouter::onGroupListUpdated()
{
    _group_controller.applyGroupManagementEffect(
        memochat::group::GroupManagementEventService::reduceGroupListUpdated(_make_group_management_event_context()),
        memochat::app::makeGroupManagementEventEffectPort(_make_group_management_event_effect_actions()));
}

void AppChatDispatcherEventRouter::onGroupInvite(qint64 groupId, QString groupCode, QString groupName, int operatorUid)
{
    _group_controller.applyGroupManagementEffect(
        memochat::group::GroupManagementEventService::reduceGroupInvite(groupId,
                                                                        groupCode,
                                                                        groupName,
                                                                        operatorUid,
                                                                        _make_group_management_event_context()),
        memochat::app::makeGroupManagementEventEffectPort(_make_group_management_event_effect_actions()));
}

void AppChatDispatcherEventRouter::onGroupApply(qint64 groupId,
                                                int applicantUid,
                                                QString applicantUserId,
                                                QString reason)
{
    _group_controller.applyGroupManagementEffect(
        memochat::group::GroupManagementEventService::reduceGroupApply(groupId, applicantUid, applicantUserId, reason),
        memochat::app::makeGroupManagementEventEffectPort(_make_group_management_event_effect_actions()));
}

void AppChatDispatcherEventRouter::onGroupMemberChanged(QJsonObject payload)
{
    const memochat::chat::GroupMemberEventRoute route =
        memochat::chat::ChatProtocolRouter::routeGroupMemberEvent(payload);
    if (route == memochat::chat::GroupMemberEventRoute::GroupReadAck)
    {
        const int selfUid = _user_mgr ? _user_mgr->GetUid() : 0;
        if (selfUid <= 0)
        {
            return;
        }
        GroupReadAckRequest request;
        request.context = _make_group_conversation_context(selfUid);
        request.payload = std::move(payload);
        _chat_controller.handleGroupReadAckEvent(request, _make_group_conversation_dependencies());
        return;
    }

    if (route == memochat::chat::GroupMemberEventRoute::GroupMessageMutation)
    {
        GroupMutationRequest request;
        request.context = _make_group_conversation_context(_user_mgr ? _user_mgr->GetUid() : 0);
        request.payload = std::move(payload);
        _chat_controller.handleGroupMessageMutation(request, _make_group_conversation_dependencies());
        return;
    }

    _group_controller.applyGroupManagementEffect(
        memochat::group::GroupManagementEventService::reduceGroupMemberChanged(payload,
                                                                               _make_group_management_event_context()),
        memochat::app::makeGroupManagementEventEffectPort(_make_group_management_event_effect_actions()));
}

void AppChatDispatcherEventRouter::onGroupChatMsg(std::shared_ptr<GroupChatMsg> msg)
{
    const IncomingMessageRouteResult result =
        _chat_controller.routeGroupIncomingMessage(_make_incoming_message_readiness(),
                                                   std::move(msg),
                                                   _make_incoming_message_dispatch());
    if (result.buffered || result.applied)
    {
        return;
    }
}

void AppChatDispatcherEventRouter::onGroupRsp(ReqId reqId, int error, QJsonObject payload)
{
    if (error != ErrorCodes::SUCCESS)
    {
        if (_group_response_handlers.handleError)
        {
            _group_response_handlers.handleError(reqId, error, payload);
        }
        return;
    }

    switch (memochat::chat::ChatProtocolRouter::routeGroupResponse(reqId))
    {
        case memochat::chat::GroupResponseRoute::Management:
            if (_group_response_handlers.handleManagement)
            {
                _group_response_handlers.handleManagement(reqId, payload);
            }
            break;
        case memochat::chat::GroupResponseRoute::History:
            if (_group_response_handlers.handleHistory)
            {
                _group_response_handlers.handleHistory(payload);
            }
            break;
        case memochat::chat::GroupResponseRoute::GroupMessageMutation:
            if (_group_response_handlers.handleGroupMessageMutation)
            {
                _group_response_handlers.handleGroupMessageMutation(reqId, payload);
            }
            break;
        case memochat::chat::GroupResponseRoute::PrivateMessageMutation:
            if (_group_response_handlers.handlePrivateMessageMutation)
            {
                _group_response_handlers.handlePrivateMessageMutation(reqId, payload);
            }
            break;
        case memochat::chat::GroupResponseRoute::PrivateForward:
            if (_group_response_handlers.handlePrivateForward)
            {
                _group_response_handlers.handlePrivateForward(payload);
            }
            break;
        case memochat::chat::GroupResponseRoute::GroupMessageAck:
            if (_group_response_handlers.handleGroupMessageAck)
            {
                _group_response_handlers.handleGroupMessageAck(reqId, payload);
            }
            break;
        case memochat::chat::GroupResponseRoute::DialogMeta:
            if (_group_response_handlers.handleDialogMeta)
            {
                _group_response_handlers.handleDialogMeta(reqId, payload);
            }
            break;
        case memochat::chat::GroupResponseRoute::Ignore:
        default:
            break;
    }
}

void AppChatDispatcherEventRouter::onPrivateHistoryRsp(QJsonObject payload)
{
    auto selfInfo = _user_mgr ? _user_mgr->GetUserInfo() : nullptr;

    PrivateHistoryResponseRequest request;
    request.payload = std::move(payload);
    request.currentPeerUid = _current_private_peer_uid ? _current_private_peer_uid() : 0;
    request.selfUid = selfInfo ? selfInfo->_uid : 0;

    const PrivateHistoryResponseResult result =
        _chat_controller.handlePrivateHistoryResponse(request, _make_private_history_response_dependencies());

    qInfo() << "Private history response handled, peer uid:" << result.peerUid
            << "current chat uid:" << request.currentPeerUid << "pending matched:" << result.pendingMatched
            << "success:" << result.success << "current dialog:" << result.currentDialog
            << "pagination:" << result.pagination << "parsed count:" << static_cast<qlonglong>(result.parsedCount)
            << "message model count:" << _chat_controller.messageCount();
}

void AppChatDispatcherEventRouter::onDialogListRsp(QJsonObject payload)
{
    _chat_controller.handleDialogListResponse(std::move(payload),
                                              _make_dialog_list_response_context(),
                                              _make_dialog_list_app_port());
}

void AppChatDispatcherEventRouter::onPrivateMsgChanged(QJsonObject payload)
{
    auto selfInfo = _user_mgr ? _user_mgr->GetUserInfo() : nullptr;
    if (!selfInfo)
    {
        return;
    }

    PrivateMessageChangedRequest request;
    request.context = _make_private_chat_event_context(selfInfo->_uid);
    request.payload = std::move(payload);
    _chat_controller.handlePrivateMessageChanged(request, _make_private_chat_event_dependencies());
}

void AppChatDispatcherEventRouter::onPrivateReadAck(QJsonObject payload)
{
    auto selfInfo = _user_mgr ? _user_mgr->GetUserInfo() : nullptr;
    if (!selfInfo)
    {
        return;
    }

    PrivateReadAckRequest request;
    request.context = _make_private_chat_event_context(selfInfo->_uid);
    request.payload = std::move(payload);
    _chat_controller.handlePrivateReadAck(request, _make_private_chat_event_dependencies());
}

void AppChatDispatcherEventRouter::onMessageStatus(QJsonObject payload)
{
    const memochat::chat::MessageStatusRoute route = memochat::chat::ChatProtocolRouter::routeMessageStatus(payload);
    if (route == memochat::chat::MessageStatusRoute::Ignore)
    {
        return;
    }

    auto selfInfo = _user_mgr ? _user_mgr->GetUserInfo() : nullptr;
    const int selfUid = selfInfo ? selfInfo->_uid : (_user_mgr ? _user_mgr->GetUid() : 0);

    if (route == memochat::chat::MessageStatusRoute::GroupMessageStatus)
    {
        GroupAckRequest request;
        request.context = _make_group_conversation_context(selfUid);
        request.payload = std::move(payload);
        _chat_controller.handleGroupMessageStatus(request, _make_group_conversation_dependencies());
        return;
    }

    PrivateMessageStatusRequest request;
    request.context = _make_private_chat_event_context(selfUid);
    request.payload = std::move(payload);
    _chat_controller.handlePrivateMessageStatus(request, _make_private_chat_event_dependencies());
}

void AppChatDispatcherEventRouter::onNotifyOffline()
{
    _connection_coordinator.onNotifyOffline();
}

void AppChatDispatcherEventRouter::onRelationBootstrapUpdated()
{
    _session_coordinator.onRelationBootstrapUpdated();
}

void AppChatDispatcherEventRouter::onCallEvent(QJsonObject payload)
{
    _call_coordinator.onCallEvent(std::move(payload));
}

void AppChatDispatcherEventRouter::onHeartbeatAck(qint64 ackAtMs)
{
    _connection_coordinator.onHeartbeatAck(ackAtMs);
}

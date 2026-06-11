#ifndef APPCHATDISPATCHEREVENTROUTER_H
#define APPCHATDISPATCHEREVENTROUTER_H

#include <QObject>
#include <QJsonObject>
#include <QtGlobal>
#include <functional>
#include <memory>

#include "ChatDialogListResponseService.h"
#include "ContactEventService.h"
#include "AppChatDispatcherGroupResponseHandlers.h"
#include "GroupConversationService.h"
#include "GroupManagementEffectPortFactory.h"
#include "GroupManagementEventService.h"
#include "IncomingMessageRouter.h"
#include "PrivateChatEventService.h"
#include "PrivateChatHistoryService.h"

class AppChatConnectionCoordinator;
class AppSessionCoordinator;
class CallCoordinator;
class ChatFeatureController;
class ContactController;
class GroupController;
class UserMgr;
struct AddFriendApply;
struct AuthInfo;
struct AuthRsp;
struct GroupChatMsg;
struct SearchInfo;
struct TextChatMsg;

class AppChatDispatcherEventRouter : public QObject
{
    Q_OBJECT

public:
    AppChatDispatcherEventRouter(
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
        QObject* parent = nullptr);

public slots:
    void onChatLoginFailed(int err);
    void onSwitchToChat();
    void onTextChatMsg(std::shared_ptr<TextChatMsg> msg);
    void onAddAuthFriend(std::shared_ptr<AuthInfo> authInfo);
    void onDeleteFriendRsp(int error, int friendUid);
    void onAuthRsp(std::shared_ptr<AuthRsp> authRsp);
    void onUserSearch(std::shared_ptr<SearchInfo> searchInfo);
    void onFriendApply(std::shared_ptr<AddFriendApply> applyInfo);
    void onGroupListUpdated();
    void onGroupInvite(qint64 groupId, QString groupCode, QString groupName, int operatorUid);
    void onGroupApply(qint64 groupId, int applicantUid, QString applicantUserId, QString reason);
    void onGroupMemberChanged(QJsonObject payload);
    void onGroupChatMsg(std::shared_ptr<GroupChatMsg> msg);
    void onGroupRsp(ReqId reqId, int error, QJsonObject payload);
    void onPrivateHistoryRsp(QJsonObject payload);
    void onDialogListRsp(QJsonObject payload);
    void onPrivateMsgChanged(QJsonObject payload);
    void onPrivateReadAck(QJsonObject payload);
    void onMessageStatus(QJsonObject payload);
    void onNotifyOffline();
    void onRelationBootstrapUpdated();
    void onCallEvent(QJsonObject payload);
    void onHeartbeatAck(qint64 ackAtMs);

private:
    AppSessionCoordinator& _session_coordinator;
    AppChatConnectionCoordinator& _connection_coordinator;
    CallCoordinator& _call_coordinator;
    ChatFeatureController& _chat_controller;
    std::shared_ptr<UserMgr> _user_mgr;
    ContactController& _contact_controller;
    GroupController& _group_controller;
    std::function<ContactEventDependencies()> _make_contact_event_dependencies;
    std::function<IncomingMessageRouterReadiness()> _make_incoming_message_readiness;
    std::function<IncomingMessageRouterDispatch()> _make_incoming_message_dispatch;
    std::function<GroupConversationContext(int)> _make_group_conversation_context;
    std::function<GroupConversationDependencies()> _make_group_conversation_dependencies;
    std::function<memochat::group::GroupManagementEventContext()> _make_group_management_event_context;
    std::function<memochat::app::GroupManagementEventEffectActions()> _make_group_management_event_effect_actions;
    memochat::app::AppChatDispatcherGroupResponseHandlers _group_response_handlers;
    std::function<ChatDialogListResponseContext()> _make_dialog_list_response_context;
    std::function<ChatDialogListAppPort()> _make_dialog_list_app_port;
    std::function<PrivateChatEventContext(int)> _make_private_chat_event_context;
    std::function<PrivateChatEventDependencies()> _make_private_chat_event_dependencies;
    std::function<int()> _current_private_peer_uid;
    std::function<PrivateHistoryResponseDependencies()> _make_private_history_response_dependencies;
};

#endif // APPCHATDISPATCHEREVENTROUTER_H

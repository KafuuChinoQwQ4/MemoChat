#include "GroupMessageService.hpp"

#include "ChatRuntime.hpp"
#include "IChatSession.hpp"
#include "GroupManagementService.hpp"
#include "GroupMembershipService.hpp"
#include "GroupMessageHistoryService.hpp"
#include "GroupMessageWorkflow.hpp"

namespace
{
MessageCommandRequest BuildGroupMessageCommandRequestLocal(const std::shared_ptr<IChatSession>& session,
                                                           short msg_id,
                                                           const std::string& msg_data)
{
    MessageCommandRequest request;
    request.request_msg_id = msg_id;
    request.payload_json = msg_data;
    request.server_name = memochat::chatruntime::SelfServerName();
    if (session)
    {
        request.session_uid = session->userId();
        request.session_id = session->sessionId();
    }
    return request;
}

void SendGroupMessageCommandResultLocal(const std::shared_ptr<IChatSession>& session,
                                        const MessageCommandResult& result)
{
    if (!session || result.response_msg_id == 0)
    {
        return;
    }
    session->send(result.payload_json, result.response_msg_id);
}
} // namespace

GroupMessageService::GroupMessageService(IMessageRepository* message_repository,
                                         IRelationRepository* relation_repository,
                                         IDeliveryGateway* delivery_gateway,
                                         IEventPublisher* event_publisher)
    : _message_repository(message_repository)
    , _relation_repository(relation_repository)
    , _delivery_gateway(delivery_gateway)
    , _event_publisher(event_publisher)
    , _membership_service(std::make_unique<GroupMembershipService>(relation_repository, delivery_gateway))
    , _workflow_service(std::make_unique<GroupMessageWorkflow>(message_repository,
                                                               relation_repository,
                                                               delivery_gateway,
                                                               event_publisher))
    , _history_service(
          std::make_unique<GroupMessageHistoryService>(message_repository, relation_repository, delivery_gateway))
    , _management_service(std::make_unique<GroupManagementService>(relation_repository, delivery_gateway))
{
}

GroupMessageService::~GroupMessageService() = default;

void GroupMessageService::BuildGroupListJson(int uid, memochat::json::JsonValue& out)
{
    _membership_service->BuildGroupListJson(uid, out);
}

MessageCommandResult GroupMessageService::CreateGroup(const MessageCommandRequest& request)
{
    return _membership_service->CreateGroup(request);
}

void GroupMessageService::HandleCreateGroup(const std::shared_ptr<IChatSession>& session,
                                            short msg_id,
                                            const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       CreateGroup(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::GetGroupList(const MessageCommandRequest& request)
{
    return _membership_service->GetGroupList(request);
}

void GroupMessageService::HandleGetGroupList(const std::shared_ptr<IChatSession>& session,
                                             short msg_id,
                                             const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       GetGroupList(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::InviteGroupMember(const MessageCommandRequest& request)
{
    return _membership_service->InviteGroupMember(request);
}

void GroupMessageService::HandleInviteGroupMember(const std::shared_ptr<IChatSession>& session,
                                                  short msg_id,
                                                  const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        InviteGroupMember(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::ApplyJoinGroup(const MessageCommandRequest& request)
{
    return _membership_service->ApplyJoinGroup(request);
}

void GroupMessageService::HandleApplyJoinGroup(const std::shared_ptr<IChatSession>& session,
                                               short msg_id,
                                               const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       ApplyJoinGroup(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::ReviewGroupApply(const MessageCommandRequest& request)
{
    return _membership_service->ReviewGroupApply(request);
}

void GroupMessageService::HandleReviewGroupApply(const std::shared_ptr<IChatSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        ReviewGroupApply(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::GroupChatMessage(const MessageCommandRequest& request)
{
    return _workflow_service->GroupChatMessage(request);
}

void GroupMessageService::HandleGroupChatMessage(const std::shared_ptr<IChatSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        GroupChatMessage(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::GroupHistory(const MessageCommandRequest& request)
{
    return _history_service->GroupHistory(request);
}

void GroupMessageService::HandleGroupHistory(const std::shared_ptr<IChatSession>& session,
                                             short msg_id,
                                             const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       GroupHistory(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::EditGroupMessage(const MessageCommandRequest& request)
{
    return _workflow_service->EditGroupMessage(request);
}

void GroupMessageService::HandleEditGroupMessage(const std::shared_ptr<IChatSession>& session,
                                                 short msg_id,
                                                 const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        EditGroupMessage(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::RevokeGroupMessage(const MessageCommandRequest& request)
{
    return _workflow_service->RevokeGroupMessage(request);
}

void GroupMessageService::HandleRevokeGroupMessage(const std::shared_ptr<IChatSession>& session,
                                                   short msg_id,
                                                   const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        RevokeGroupMessage(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::ForwardGroupMessage(const MessageCommandRequest& request)
{
    return _workflow_service->ForwardGroupMessage(request);
}

void GroupMessageService::HandleForwardGroupMessage(const std::shared_ptr<IChatSession>& session,
                                                    short msg_id,
                                                    const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        ForwardGroupMessage(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::GroupReadAck(const MessageCommandRequest& request)
{
    return _history_service->GroupReadAck(request);
}

void GroupMessageService::HandleGroupReadAck(const std::shared_ptr<IChatSession>& session,
                                             short msg_id,
                                             const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       GroupReadAck(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::UpdateGroupAnnouncement(const MessageCommandRequest& request)
{
    return _management_service->UpdateGroupAnnouncement(request);
}

void GroupMessageService::HandleUpdateGroupAnnouncement(const std::shared_ptr<IChatSession>& session,
                                                        short msg_id,
                                                        const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        UpdateGroupAnnouncement(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::UpdateGroupIcon(const MessageCommandRequest& request)
{
    return _management_service->UpdateGroupIcon(request);
}

void GroupMessageService::HandleUpdateGroupIcon(const std::shared_ptr<IChatSession>& session,
                                                short msg_id,
                                                const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        UpdateGroupIcon(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::SetGroupAdmin(const MessageCommandRequest& request)
{
    return _management_service->SetGroupAdmin(request);
}

void GroupMessageService::HandleSetGroupAdmin(const std::shared_ptr<IChatSession>& session,
                                              short msg_id,
                                              const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       SetGroupAdmin(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::MuteGroupMember(const MessageCommandRequest& request)
{
    return _management_service->MuteGroupMember(request);
}

void GroupMessageService::HandleMuteGroupMember(const std::shared_ptr<IChatSession>& session,
                                                short msg_id,
                                                const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        MuteGroupMember(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::KickGroupMember(const MessageCommandRequest& request)
{
    return _management_service->KickGroupMember(request);
}

void GroupMessageService::HandleKickGroupMember(const std::shared_ptr<IChatSession>& session,
                                                short msg_id,
                                                const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(
        session,
        KickGroupMember(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::QuitGroup(const MessageCommandRequest& request)
{
    return _management_service->QuitGroup(request);
}

void GroupMessageService::HandleQuitGroup(const std::shared_ptr<IChatSession>& session,
                                          short msg_id,
                                          const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       QuitGroup(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

MessageCommandResult GroupMessageService::DissolveGroup(const MessageCommandRequest& request)
{
    return _management_service->DissolveGroup(request);
}

void GroupMessageService::HandleDissolveGroup(const std::shared_ptr<IChatSession>& session,
                                              short msg_id,
                                              const std::string& msg_data)
{
    SendGroupMessageCommandResultLocal(session,
                                       DissolveGroup(BuildGroupMessageCommandRequestLocal(session, msg_id, msg_data)));
}

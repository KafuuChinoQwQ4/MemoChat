#pragma once

#include "ports/IDeliveryGateway.hpp"
#include "ports/IEventPublisher.hpp"
#include "ports/IGroupMessageService.hpp"
#include "ports/IMessageRepository.hpp"
#include "ports/IRelationRepository.hpp"
#include <memory>
#include <string>

class CSession;
class GroupMessageService : public IGroupMessageService
{
public:
    GroupMessageService(IMessageRepository* message_repository,
                        IRelationRepository* relation_repository,
                        IDeliveryGateway* delivery_gateway,
                        IEventPublisher* event_publisher);

    void BuildGroupListJson(int uid, memochat::json::JsonValue& out) override;
    MessageCommandResult CreateGroup(const MessageCommandRequest& request) override;
    MessageCommandResult GetGroupList(const MessageCommandRequest& request) override;
    MessageCommandResult InviteGroupMember(const MessageCommandRequest& request) override;
    MessageCommandResult ApplyJoinGroup(const MessageCommandRequest& request) override;
    MessageCommandResult ReviewGroupApply(const MessageCommandRequest& request) override;
    MessageCommandResult GroupChatMessage(const MessageCommandRequest& request) override;
    MessageCommandResult GroupHistory(const MessageCommandRequest& request) override;
    MessageCommandResult GroupReadAck(const MessageCommandRequest& request) override;
    MessageCommandResult EditGroupMessage(const MessageCommandRequest& request) override;
    MessageCommandResult RevokeGroupMessage(const MessageCommandRequest& request) override;
    MessageCommandResult ForwardGroupMessage(const MessageCommandRequest& request) override;
    MessageCommandResult UpdateGroupAnnouncement(const MessageCommandRequest& request) override;
    MessageCommandResult UpdateGroupIcon(const MessageCommandRequest& request) override;
    MessageCommandResult SetGroupAdmin(const MessageCommandRequest& request) override;
    MessageCommandResult MuteGroupMember(const MessageCommandRequest& request) override;
    MessageCommandResult KickGroupMember(const MessageCommandRequest& request) override;
    MessageCommandResult QuitGroup(const MessageCommandRequest& request) override;
    MessageCommandResult DissolveGroup(const MessageCommandRequest& request) override;
    void
    HandleCreateGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleGetGroupList(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandleInviteGroupMember(const std::shared_ptr<CSession>& session,
                                 short msg_id,
                                 const std::string& msg_data) override;
    void
    HandleApplyJoinGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandleReviewGroupApply(const std::shared_ptr<CSession>& session,
                                short msg_id,
                                const std::string& msg_data) override;
    void HandleGroupChatMessage(const std::shared_ptr<CSession>& session,
                                short msg_id,
                                const std::string& msg_data) override;
    void
    HandleGroupHistory(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandleEditGroupMessage(const std::shared_ptr<CSession>& session,
                                short msg_id,
                                const std::string& msg_data) override;
    void HandleRevokeGroupMessage(const std::shared_ptr<CSession>& session,
                                  short msg_id,
                                  const std::string& msg_data) override;
    void HandleForwardGroupMessage(const std::shared_ptr<CSession>& session,
                                   short msg_id,
                                   const std::string& msg_data) override;
    void
    HandleGroupReadAck(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandleUpdateGroupAnnouncement(const std::shared_ptr<CSession>& session,
                                       short msg_id,
                                       const std::string& msg_data) override;
    void
    HandleUpdateGroupIcon(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleSetGroupAdmin(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleMuteGroupMember(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleKickGroupMember(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandleQuitGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleDissolveGroup(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;

private:
    IMessageRepository* _message_repository = nullptr;
    IRelationRepository* _relation_repository = nullptr;
    IDeliveryGateway* _delivery_gateway = nullptr;
    IEventPublisher* _event_publisher = nullptr;
};

#pragma once

#include "MessageGrpcClient.hpp"
#include "ports/IGroupMessageService.hpp"
#include "ports/IPrivateMessageService.hpp"

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

class MessageGrpcServiceAdapter final
    : public IPrivateMessageService
    , public IGroupMessageService
{
public:
    explicit MessageGrpcServiceAdapter(const std::string& endpoint,
                                       std::chrono::milliseconds timeout = std::chrono::seconds(2));
    explicit MessageGrpcServiceAdapter(std::shared_ptr<grpc::Channel> channel,
                                       std::chrono::milliseconds timeout = std::chrono::seconds(2));

    MessageCommandResult TextChatMessage(const MessageCommandRequest& request) override;
    MessageCommandResult ForwardPrivateMessage(const MessageCommandRequest& request) override;
    MessageCommandResult PrivateReadAck(const MessageCommandRequest& request) override;
    MessageCommandResult EditPrivateMessage(const MessageCommandRequest& request) override;
    MessageCommandResult RevokePrivateMessage(const MessageCommandRequest& request) override;
    MessageCommandResult PrivateHistory(const MessageCommandRequest& request) override;
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
    HandleTextChatMessage(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandleForwardPrivateMessage(const std::shared_ptr<CSession>& session,
                                     short msg_id,
                                     const std::string& msg_data) override;
    void
    HandlePrivateReadAck(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandleEditPrivateMessage(const std::shared_ptr<CSession>& session,
                                  short msg_id,
                                  const std::string& msg_data) override;
    void HandleRevokePrivateMessage(const std::shared_ptr<CSession>& session,
                                    short msg_id,
                                    const std::string& msg_data) override;
    void
    HandlePrivateHistory(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;

    void BuildGroupListJson(int uid, memochat::json::JsonValue& out) override;
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
    MessageCommandRequest BuildSessionCommandRequest(const std::shared_ptr<CSession>& session,
                                                     short msg_id,
                                                     const std::string& msg_data) const;
    void SendSessionCommandResult(const std::shared_ptr<CSession>& session, const MessageCommandResult& result) const;

    MessageGrpcClient _client;
};

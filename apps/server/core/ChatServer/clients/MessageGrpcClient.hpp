#pragma once

#include "common/proto/chat_internal.grpc.pb.h"
#include "ports/IMessageCommandService.hpp"

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

class MessageGrpcClient final
    : public IPrivateMessageCommandService
    , public IGroupMessageCommandService
{
public:
    enum class PrivateCommandRpc
    {
        SendTextChatMessage,
        ForwardPrivateMessage,
        PrivateReadAck,
        EditPrivateMessage,
        RevokePrivateMessage,
        PrivateHistory,
    };

    enum class GroupCommandRpc
    {
        CreateGroup,
        GetGroupList,
        InviteGroupMember,
        ApplyJoinGroup,
        ReviewGroupApply,
        GroupChatMessage,
        GroupHistory,
        EditGroupMessage,
        RevokeGroupMessage,
        ForwardGroupMessage,
        GroupReadAck,
        UpdateGroupAnnouncement,
        UpdateGroupIcon,
        SetGroupAdmin,
        MuteGroupMember,
        KickGroupMember,
        QuitGroup,
        DissolveGroup,
    };

    explicit MessageGrpcClient(const std::string& endpoint,
                               std::chrono::milliseconds timeout = std::chrono::seconds(2));
    explicit MessageGrpcClient(std::shared_ptr<grpc::Channel> channel,
                               std::chrono::milliseconds timeout = std::chrono::seconds(2));

    MessageCommandResult TextChatMessage(const MessageCommandRequest& request) override;
    MessageCommandResult ForwardPrivateMessage(const MessageCommandRequest& request) override;
    MessageCommandResult PrivateReadAck(const MessageCommandRequest& request) override;
    MessageCommandResult EditPrivateMessage(const MessageCommandRequest& request) override;
    MessageCommandResult RevokePrivateMessage(const MessageCommandRequest& request) override;
    MessageCommandResult PrivateHistory(const MessageCommandRequest& request) override;
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

private:
    void CallGroupQuery(int uid, memochat::json::JsonValue& out);
    MessageCommandResult CallPrivateCommand(PrivateCommandRpc rpc, const MessageCommandRequest& request);
    MessageCommandResult CallGroupCommand(GroupCommandRpc rpc, const MessageCommandRequest& request);

    std::unique_ptr<chatinternal::ChatPrivateMessageInternalService::Stub> _private_stub;
    std::unique_ptr<chatinternal::ChatGroupMessageInternalService::Stub> _group_stub;
    std::chrono::milliseconds _timeout;
};

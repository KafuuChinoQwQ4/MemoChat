#pragma once

#include "common/proto/chat_internal.grpc.pb.h"
#include "ports/IMessageCommandService.h"

class ChatMessageInternalGrpcService final
    : public chatinternal::ChatPrivateMessageInternalService::Service
    , public chatinternal::ChatGroupMessageInternalService::Service
{
public:
    ChatMessageInternalGrpcService(IPrivateMessageCommandService* private_message_service,
                                   IGroupMessageCommandService* group_message_service);

    grpc::Status SendTextChatMessage(grpc::ServerContext* context,
                                     const chatinternal::JsonPayloadRequest* request,
                                     chatinternal::JsonPayloadResponse* response) override;
    grpc::Status ForwardPrivateMessage(grpc::ServerContext* context,
                                       const chatinternal::JsonPayloadRequest* request,
                                       chatinternal::JsonPayloadResponse* response) override;
    grpc::Status PrivateReadAck(grpc::ServerContext* context,
                                const chatinternal::JsonPayloadRequest* request,
                                chatinternal::JsonPayloadResponse* response) override;
    grpc::Status EditPrivateMessage(grpc::ServerContext* context,
                                    const chatinternal::JsonPayloadRequest* request,
                                    chatinternal::JsonPayloadResponse* response) override;
    grpc::Status RevokePrivateMessage(grpc::ServerContext* context,
                                      const chatinternal::JsonPayloadRequest* request,
                                      chatinternal::JsonPayloadResponse* response) override;
    grpc::Status PrivateHistory(grpc::ServerContext* context,
                                const chatinternal::JsonPayloadRequest* request,
                                chatinternal::JsonPayloadResponse* response) override;
    grpc::Status BuildGroupList(grpc::ServerContext* context,
                                const chatinternal::BootstrapRequest* request,
                                chatinternal::BootstrapResponse* response) override;
    grpc::Status CreateGroup(grpc::ServerContext* context,
                             const chatinternal::JsonPayloadRequest* request,
                             chatinternal::JsonPayloadResponse* response) override;
    grpc::Status GetGroupList(grpc::ServerContext* context,
                              const chatinternal::JsonPayloadRequest* request,
                              chatinternal::JsonPayloadResponse* response) override;
    grpc::Status InviteGroupMember(grpc::ServerContext* context,
                                   const chatinternal::JsonPayloadRequest* request,
                                   chatinternal::JsonPayloadResponse* response) override;
    grpc::Status ApplyJoinGroup(grpc::ServerContext* context,
                                const chatinternal::JsonPayloadRequest* request,
                                chatinternal::JsonPayloadResponse* response) override;
    grpc::Status ReviewGroupApply(grpc::ServerContext* context,
                                  const chatinternal::JsonPayloadRequest* request,
                                  chatinternal::JsonPayloadResponse* response) override;
    grpc::Status GroupChatMessage(grpc::ServerContext* context,
                                  const chatinternal::JsonPayloadRequest* request,
                                  chatinternal::JsonPayloadResponse* response) override;
    grpc::Status GroupHistory(grpc::ServerContext* context,
                              const chatinternal::JsonPayloadRequest* request,
                              chatinternal::JsonPayloadResponse* response) override;
    grpc::Status EditGroupMessage(grpc::ServerContext* context,
                                  const chatinternal::JsonPayloadRequest* request,
                                  chatinternal::JsonPayloadResponse* response) override;
    grpc::Status RevokeGroupMessage(grpc::ServerContext* context,
                                    const chatinternal::JsonPayloadRequest* request,
                                    chatinternal::JsonPayloadResponse* response) override;
    grpc::Status ForwardGroupMessage(grpc::ServerContext* context,
                                     const chatinternal::JsonPayloadRequest* request,
                                     chatinternal::JsonPayloadResponse* response) override;
    grpc::Status GroupReadAck(grpc::ServerContext* context,
                              const chatinternal::JsonPayloadRequest* request,
                              chatinternal::JsonPayloadResponse* response) override;
    grpc::Status UpdateGroupAnnouncement(grpc::ServerContext* context,
                                         const chatinternal::JsonPayloadRequest* request,
                                         chatinternal::JsonPayloadResponse* response) override;
    grpc::Status UpdateGroupIcon(grpc::ServerContext* context,
                                 const chatinternal::JsonPayloadRequest* request,
                                 chatinternal::JsonPayloadResponse* response) override;
    grpc::Status SetGroupAdmin(grpc::ServerContext* context,
                               const chatinternal::JsonPayloadRequest* request,
                               chatinternal::JsonPayloadResponse* response) override;
    grpc::Status MuteGroupMember(grpc::ServerContext* context,
                                 const chatinternal::JsonPayloadRequest* request,
                                 chatinternal::JsonPayloadResponse* response) override;
    grpc::Status KickGroupMember(grpc::ServerContext* context,
                                 const chatinternal::JsonPayloadRequest* request,
                                 chatinternal::JsonPayloadResponse* response) override;
    grpc::Status QuitGroup(grpc::ServerContext* context,
                           const chatinternal::JsonPayloadRequest* request,
                           chatinternal::JsonPayloadResponse* response) override;
    grpc::Status DissolveGroup(grpc::ServerContext* context,
                               const chatinternal::JsonPayloadRequest* request,
                               chatinternal::JsonPayloadResponse* response) override;

private:
    grpc::Status BuildPrivateCommandResponse(
        const chatinternal::JsonPayloadRequest* request,
        chatinternal::JsonPayloadResponse* response,
        MessageCommandResult (IPrivateMessageCommandService::*handler)(const MessageCommandRequest&));
    grpc::Status BuildGroupCommandResponse(
        const chatinternal::JsonPayloadRequest* request,
        chatinternal::JsonPayloadResponse* response,
        MessageCommandResult (IGroupMessageCommandService::*handler)(const MessageCommandRequest&));

    IPrivateMessageCommandService* _private_message_service = nullptr;
    IGroupMessageCommandService* _group_message_service = nullptr;
};

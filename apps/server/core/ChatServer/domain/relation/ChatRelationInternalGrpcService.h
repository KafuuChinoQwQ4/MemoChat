#pragma once

#include "common/proto/chat_internal.grpc.pb.h"
#include "ports/IRelationService.h"

class ChatRelationInternalGrpcService final : public chatinternal::ChatRelationInternalService::Service
{
public:
    explicit ChatRelationInternalGrpcService(IRelationService* relation_service);

    grpc::Status AppendRelationBootstrap(grpc::ServerContext* context,
                                         const chatinternal::BootstrapRequest* request,
                                         chatinternal::BootstrapResponse* response) override;
    grpc::Status BuildDialogList(grpc::ServerContext* context,
                                 const chatinternal::BootstrapRequest* request,
                                 chatinternal::BootstrapResponse* response) override;
    grpc::Status SearchUser(grpc::ServerContext* context,
                            const chatinternal::JsonPayloadRequest* request,
                            chatinternal::JsonPayloadResponse* response) override;
    grpc::Status AddFriendApply(grpc::ServerContext* context,
                                const chatinternal::JsonPayloadRequest* request,
                                chatinternal::JsonPayloadResponse* response) override;
    grpc::Status AuthFriendApply(grpc::ServerContext* context,
                                 const chatinternal::JsonPayloadRequest* request,
                                 chatinternal::JsonPayloadResponse* response) override;
    grpc::Status DeleteFriend(grpc::ServerContext* context,
                              const chatinternal::JsonPayloadRequest* request,
                              chatinternal::JsonPayloadResponse* response) override;
    grpc::Status GetDialogList(grpc::ServerContext* context,
                               const chatinternal::JsonPayloadRequest* request,
                               chatinternal::JsonPayloadResponse* response) override;
    grpc::Status SyncDraft(grpc::ServerContext* context,
                           const chatinternal::JsonPayloadRequest* request,
                           chatinternal::JsonPayloadResponse* response) override;
    grpc::Status PinDialog(grpc::ServerContext* context,
                           const chatinternal::JsonPayloadRequest* request,
                           chatinternal::JsonPayloadResponse* response) override;
    grpc::Status FilterFriendUids(grpc::ServerContext* context,
                                  const chatinternal::JsonPayloadRequest* request,
                                  chatinternal::JsonPayloadResponse* response) override;

private:
    grpc::Status BuildBootstrapResponse(const chatinternal::BootstrapRequest* request,
                                        chatinternal::BootstrapResponse* response,
                                        void (IRelationQueryService::*builder)(int, memochat::json::JsonValue&));
    grpc::Status
    BuildCommandResponse(const chatinternal::JsonPayloadRequest* request,
                         chatinternal::JsonPayloadResponse* response,
                         RelationCommandResult (IRelationCommandService::*handler)(const RelationCommandRequest&));

    IRelationService* _relation_service = nullptr;
};

#pragma once

#include "common/proto/chat_internal.grpc.pb.h"
#include "ports/IRelationService.hpp"

#include <string>

enum class RelationGrpcAccessMode
{
    ReadWrite,
    QueryOnly,
};

struct RelationGrpcAuthTokens
{
    std::string chat;
    std::string call;
    std::string moments;
};

enum class RelationGrpcCaller
{
    Chat,
    Call,
    Moments,
};

enum class RelationGrpcAuthMode
{
    Required,
    DisabledForTests,
};

class ChatRelationInternalGrpcService final : public chatinternal::ChatRelationInternalService::Service
{
public:
    explicit ChatRelationInternalGrpcService(IRelationService* relation_service,
                                             RelationGrpcAccessMode access_mode = RelationGrpcAccessMode::ReadWrite,
                                             RelationGrpcAuthTokens auth_tokens = {},
                                             RelationGrpcAuthMode auth_mode = RelationGrpcAuthMode::Required);

    grpc::Status AppendRelationBootstrap(grpc::ServerContext* context,
                                         const chatinternal::BootstrapRequest* request,
                                         chatinternal::BootstrapResponse* response) override;
    grpc::Status BuildDialogList(grpc::ServerContext* context,
                                 const chatinternal::BootstrapRequest* request,
                                 chatinternal::BootstrapResponse* response) override;
    grpc::Status CheckFriendship(grpc::ServerContext* context,
                                 const chatinternal::FriendshipRequest* request,
                                 chatinternal::FriendshipResponse* response) override;
    grpc::Status CheckHealth(grpc::ServerContext* context,
                             const chatinternal::RelationHealthRequest* request,
                             chatinternal::RelationHealthResponse* response) override;
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
    grpc::Status RejectRestrictedCommand(chatinternal::JsonPayloadResponse* response) const;
    grpc::Status Authorize(grpc::ServerContext* context, RelationGrpcCaller caller) const;
    grpc::Status AuthorizeHealth(grpc::ServerContext* context) const;

    IRelationService* _relation_service = nullptr;
    RelationGrpcAccessMode _access_mode = RelationGrpcAccessMode::ReadWrite;
    RelationGrpcAuthTokens _auth_tokens;
    RelationGrpcAuthMode _auth_mode = RelationGrpcAuthMode::Required;
};

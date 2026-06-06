#include "ChatRelationInternalGrpcService.h"

#include "const.h"

namespace
{
std::string CompactJson(const memochat::json::JsonValue& value)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, value);
}

RelationCommandRequest BuildCommandRequest(const chatinternal::JsonPayloadRequest& request)
{
    RelationCommandRequest command_request;
    command_request.request_msg_id = static_cast<short>(request.tcp_msg_id());
    command_request.payload_json = request.payload_json();
    command_request.session_uid = request.session().uid();
    command_request.session_id = request.session().session_id();
    command_request.server_name = request.session().server_name();
    command_request.trace_id = request.session().trace_id();
    return command_request;
}
} // namespace

ChatRelationInternalGrpcService::ChatRelationInternalGrpcService(IRelationService* relation_service)
    : _relation_service(relation_service)
{
}

grpc::Status ChatRelationInternalGrpcService::AppendRelationBootstrap(grpc::ServerContext*,
                                                                      const chatinternal::BootstrapRequest* request,
                                                                      chatinternal::BootstrapResponse* response)
{
    return BuildBootstrapResponse(request, response, &IRelationQueryService::AppendRelationBootstrapJson);
}

grpc::Status ChatRelationInternalGrpcService::BuildDialogList(grpc::ServerContext*,
                                                              const chatinternal::BootstrapRequest* request,
                                                              chatinternal::BootstrapResponse* response)
{
    return BuildBootstrapResponse(request, response, &IRelationQueryService::BuildDialogListJson);
}

grpc::Status ChatRelationInternalGrpcService::SearchUser(grpc::ServerContext*,
                                                         const chatinternal::JsonPayloadRequest* request,
                                                         chatinternal::JsonPayloadResponse* response)
{
    return BuildCommandResponse(request, response, &IRelationCommandService::SearchUser);
}

grpc::Status ChatRelationInternalGrpcService::AddFriendApply(grpc::ServerContext*,
                                                             const chatinternal::JsonPayloadRequest* request,
                                                             chatinternal::JsonPayloadResponse* response)
{
    return BuildCommandResponse(request, response, &IRelationCommandService::AddFriendApply);
}

grpc::Status ChatRelationInternalGrpcService::AuthFriendApply(grpc::ServerContext*,
                                                              const chatinternal::JsonPayloadRequest* request,
                                                              chatinternal::JsonPayloadResponse* response)
{
    return BuildCommandResponse(request, response, &IRelationCommandService::AuthFriendApply);
}

grpc::Status ChatRelationInternalGrpcService::DeleteFriend(grpc::ServerContext*,
                                                           const chatinternal::JsonPayloadRequest* request,
                                                           chatinternal::JsonPayloadResponse* response)
{
    return BuildCommandResponse(request, response, &IRelationCommandService::DeleteFriend);
}

grpc::Status ChatRelationInternalGrpcService::GetDialogList(grpc::ServerContext*,
                                                            const chatinternal::JsonPayloadRequest* request,
                                                            chatinternal::JsonPayloadResponse* response)
{
    return BuildCommandResponse(request, response, &IRelationCommandService::GetDialogList);
}

grpc::Status ChatRelationInternalGrpcService::SyncDraft(grpc::ServerContext*,
                                                        const chatinternal::JsonPayloadRequest* request,
                                                        chatinternal::JsonPayloadResponse* response)
{
    return BuildCommandResponse(request, response, &IRelationCommandService::SyncDraft);
}

grpc::Status ChatRelationInternalGrpcService::PinDialog(grpc::ServerContext*,
                                                        const chatinternal::JsonPayloadRequest* request,
                                                        chatinternal::JsonPayloadResponse* response)
{
    return BuildCommandResponse(request, response, &IRelationCommandService::PinDialog);
}

grpc::Status ChatRelationInternalGrpcService::BuildBootstrapResponse(
    const chatinternal::BootstrapRequest* request,
    chatinternal::BootstrapResponse* response,
    void (IRelationQueryService::*builder)(int, memochat::json::JsonValue&))
{
    if (!request || !response)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "missing bootstrap request or response");
    }
    if (!_relation_service)
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "relation service is not configured");
    }
    const auto uid = request->uid();
    if (uid <= 0)
    {
        response->set_error(ErrorCodes::UidInvalid);
        response->set_payload_json("{}");
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "uid must be positive");
    }

    memochat::json::JsonValue payload(memochat::json::object_t{});
    payload["uid"] = uid;
    (void) request->trace_id();
    (_relation_service->*builder)(uid, payload);

    response->set_error(ErrorCodes::Success);
    response->set_payload_json(CompactJson(payload));
    return grpc::Status::OK;
}

grpc::Status ChatRelationInternalGrpcService::BuildCommandResponse(
    const chatinternal::JsonPayloadRequest* request,
    chatinternal::JsonPayloadResponse* response,
    RelationCommandResult (IRelationCommandService::*handler)(const RelationCommandRequest&))
{
    if (!request || !response)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "missing relation command request or response");
    }
    if (!_relation_service)
    {
        if (response)
        {
            response->set_error(ErrorCodes::RPCFailed);
            response->set_payload_json("{}");
        }
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "relation service is not configured");
    }

    const auto result = (_relation_service->*handler)(BuildCommandRequest(*request));
    response->set_error(ErrorCodes::Success);
    response->set_tcp_msg_id(result.response_msg_id);
    response->set_payload_json(result.payload_json);
    return grpc::Status::OK;
}

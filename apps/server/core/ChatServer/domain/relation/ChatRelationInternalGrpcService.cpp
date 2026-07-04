#include "ChatRelationInternalGrpcService.hpp"

#include "const.hpp"

import memochat.chat.relation_internal_grpc_service_algorithms;

namespace relation_internal_grpc_modules = memochat::chat::relation_internal_grpc_service::modules;

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
    command_request.request_msg_id = relation_internal_grpc_modules::TcpMessageId(request.tcp_msg_id());
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

grpc::Status ChatRelationInternalGrpcService::FilterFriendUids(grpc::ServerContext*,
                                                               const chatinternal::JsonPayloadRequest* request,
                                                               chatinternal::JsonPayloadResponse* response)
{
    return BuildCommandResponse(request, response, &IRelationCommandService::FilterFriendUids);
}

grpc::Status ChatRelationInternalGrpcService::BuildBootstrapResponse(
    const chatinternal::BootstrapRequest* request,
    chatinternal::BootstrapResponse* response,
    void (IRelationQueryService::*builder)(int, memochat::json::JsonValue&))
{
    if (relation_internal_grpc_modules::ShouldReportMissingRequestOrResponse(request != nullptr, response != nullptr))
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            relation_internal_grpc_modules::MissingBootstrapRequestMessage());
    }
    if (relation_internal_grpc_modules::ShouldReportMissingRelationService(_relation_service != nullptr))
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                            relation_internal_grpc_modules::RelationServiceNotConfiguredMessage());
    }
    const auto uid = request->uid();
    if (relation_internal_grpc_modules::ShouldReportInvalidUid(uid))
    {
        response->set_error(ErrorCodes::UidInvalid);
        response->set_payload_json(relation_internal_grpc_modules::DefaultPayloadJson());
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            relation_internal_grpc_modules::UidMustBePositiveMessage());
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
    if (relation_internal_grpc_modules::ShouldReportMissingRequestOrResponse(request != nullptr, response != nullptr))
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            relation_internal_grpc_modules::MissingRelationCommandRequestMessage());
    }
    if (relation_internal_grpc_modules::ShouldReportMissingRelationService(_relation_service != nullptr))
    {
        if (response)
        {
            response->set_error(ErrorCodes::RPCFailed);
            response->set_payload_json(relation_internal_grpc_modules::DefaultPayloadJson());
        }
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                            relation_internal_grpc_modules::RelationServiceNotConfiguredMessage());
    }

    const auto result = (_relation_service->*handler)(BuildCommandRequest(*request));
    response->set_error(ErrorCodes::Success);
    response->set_tcp_msg_id(result.response_msg_id);
    response->set_payload_json(result.payload_json);
    return grpc::Status::OK;
}

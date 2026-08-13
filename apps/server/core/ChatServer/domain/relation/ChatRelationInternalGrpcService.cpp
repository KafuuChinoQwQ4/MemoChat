#include "ChatRelationInternalGrpcService.hpp"

#include "auth/RelationGrpcAuth.hpp"
#include "const.hpp"
#include "logging/GrpcTrace.hpp"

#include <utility>

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

ChatRelationInternalGrpcService::ChatRelationInternalGrpcService(IRelationService* relation_service,
                                                                 RelationGrpcAccessMode access_mode,
                                                                 RelationGrpcAuthTokens auth_tokens,
                                                                 RelationGrpcAuthMode auth_mode)
    : _relation_service(relation_service)
    , _access_mode(access_mode)
    , _auth_tokens(std::move(auth_tokens))
    , _auth_mode(auth_mode)
{
}

grpc::Status ChatRelationInternalGrpcService::AppendRelationBootstrap(grpc::ServerContext* context,
                                                                      const chatinternal::BootstrapRequest* request,
                                                                      chatinternal::BootstrapResponse* response)
{
    if (const auto auth = Authorize(context, RelationGrpcCaller::Chat); !auth.ok())
    {
        return auth;
    }
    return BuildBootstrapResponse(request, response, &IRelationQueryService::AppendRelationBootstrapJson);
}

grpc::Status ChatRelationInternalGrpcService::BuildDialogList(grpc::ServerContext* context,
                                                              const chatinternal::BootstrapRequest* request,
                                                              chatinternal::BootstrapResponse* response)
{
    if (const auto auth = Authorize(context, RelationGrpcCaller::Chat); !auth.ok())
    {
        return auth;
    }
    return BuildBootstrapResponse(request, response, &IRelationQueryService::BuildDialogListJson);
}

grpc::Status ChatRelationInternalGrpcService::CheckFriendship(grpc::ServerContext* context,
                                                              const chatinternal::FriendshipRequest* request,
                                                              chatinternal::FriendshipResponse* response)
{
    memolog::BindGrpcTraceContext(context);
    if (const auto auth = Authorize(context,
                                    _access_mode == RelationGrpcAccessMode::QueryOnly ? RelationGrpcCaller::Call
                                                                                      : RelationGrpcCaller::Chat);
        !auth.ok())
    {
        return auth;
    }
    if (relation_internal_grpc_modules::ShouldReportMissingRequestOrResponse(request != nullptr, response != nullptr))
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            relation_internal_grpc_modules::MissingFriendshipRequestMessage());
    }
    if (relation_internal_grpc_modules::ShouldReportMissingRelationService(_relation_service != nullptr))
    {
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                            relation_internal_grpc_modules::RelationServiceNotConfiguredMessage());
    }
    if (relation_internal_grpc_modules::ShouldReportInvalidFriendship(request->uid(), request->peer_uid()))
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            relation_internal_grpc_modules::FriendshipUidsMustBeDistinctAndPositiveMessage());
    }

    response->set_are_friends(_relation_service->AreUsersFriends(request->uid(), request->peer_uid()));
    return grpc::Status::OK;
}

grpc::Status ChatRelationInternalGrpcService::CheckHealth(grpc::ServerContext* context,
                                                          const chatinternal::RelationHealthRequest* request,
                                                          chatinternal::RelationHealthResponse* response)
{
    memolog::BindGrpcTraceContext(context);
    if (const auto auth = AuthorizeHealth(context); !auth.ok())
    {
        return auth;
    }
    if (request == nullptr || response == nullptr)
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "relation health request/response is missing");
    }
    if (_relation_service == nullptr)
    {
        response->set_ready(false);
        response->set_error("relation dependency is not configured");
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "relation dependency is not configured");
    }

    std::string health_error;
    const bool ready = _relation_service->CheckHealth(&health_error);
    response->set_ready(ready);
    if (!ready)
    {
        // Do not expose database connection details over the internal probe.
        response->set_error("relation dependency unavailable");
        return grpc::Status(grpc::StatusCode::UNAVAILABLE, "relation dependency unavailable");
    }
    return grpc::Status::OK;
}

grpc::Status ChatRelationInternalGrpcService::SearchUser(grpc::ServerContext* context,
                                                         const chatinternal::JsonPayloadRequest* request,
                                                         chatinternal::JsonPayloadResponse* response)
{
    if (_access_mode == RelationGrpcAccessMode::QueryOnly)
    {
        return RejectRestrictedCommand(response);
    }
    if (const auto auth = Authorize(context, RelationGrpcCaller::Chat); !auth.ok())
    {
        return auth;
    }
    return BuildCommandResponse(request, response, &IRelationCommandService::SearchUser);
}

grpc::Status ChatRelationInternalGrpcService::AddFriendApply(grpc::ServerContext* context,
                                                             const chatinternal::JsonPayloadRequest* request,
                                                             chatinternal::JsonPayloadResponse* response)
{
    if (_access_mode == RelationGrpcAccessMode::QueryOnly)
    {
        return RejectRestrictedCommand(response);
    }
    if (const auto auth = Authorize(context, RelationGrpcCaller::Chat); !auth.ok())
    {
        return auth;
    }
    return BuildCommandResponse(request, response, &IRelationCommandService::AddFriendApply);
}

grpc::Status ChatRelationInternalGrpcService::AuthFriendApply(grpc::ServerContext* context,
                                                              const chatinternal::JsonPayloadRequest* request,
                                                              chatinternal::JsonPayloadResponse* response)
{
    if (_access_mode == RelationGrpcAccessMode::QueryOnly)
    {
        return RejectRestrictedCommand(response);
    }
    if (const auto auth = Authorize(context, RelationGrpcCaller::Chat); !auth.ok())
    {
        return auth;
    }
    return BuildCommandResponse(request, response, &IRelationCommandService::AuthFriendApply);
}

grpc::Status ChatRelationInternalGrpcService::DeleteFriend(grpc::ServerContext* context,
                                                           const chatinternal::JsonPayloadRequest* request,
                                                           chatinternal::JsonPayloadResponse* response)
{
    if (_access_mode == RelationGrpcAccessMode::QueryOnly)
    {
        return RejectRestrictedCommand(response);
    }
    if (const auto auth = Authorize(context, RelationGrpcCaller::Chat); !auth.ok())
    {
        return auth;
    }
    return BuildCommandResponse(request, response, &IRelationCommandService::DeleteFriend);
}

grpc::Status ChatRelationInternalGrpcService::GetDialogList(grpc::ServerContext* context,
                                                            const chatinternal::JsonPayloadRequest* request,
                                                            chatinternal::JsonPayloadResponse* response)
{
    if (_access_mode == RelationGrpcAccessMode::QueryOnly)
    {
        return RejectRestrictedCommand(response);
    }
    if (const auto auth = Authorize(context, RelationGrpcCaller::Chat); !auth.ok())
    {
        return auth;
    }
    return BuildCommandResponse(request, response, &IRelationCommandService::GetDialogList);
}

grpc::Status ChatRelationInternalGrpcService::SyncDraft(grpc::ServerContext* context,
                                                        const chatinternal::JsonPayloadRequest* request,
                                                        chatinternal::JsonPayloadResponse* response)
{
    if (_access_mode == RelationGrpcAccessMode::QueryOnly)
    {
        return RejectRestrictedCommand(response);
    }
    if (const auto auth = Authorize(context, RelationGrpcCaller::Chat); !auth.ok())
    {
        return auth;
    }
    return BuildCommandResponse(request, response, &IRelationCommandService::SyncDraft);
}

grpc::Status ChatRelationInternalGrpcService::PinDialog(grpc::ServerContext* context,
                                                        const chatinternal::JsonPayloadRequest* request,
                                                        chatinternal::JsonPayloadResponse* response)
{
    if (_access_mode == RelationGrpcAccessMode::QueryOnly)
    {
        return RejectRestrictedCommand(response);
    }
    if (const auto auth = Authorize(context, RelationGrpcCaller::Chat); !auth.ok())
    {
        return auth;
    }
    return BuildCommandResponse(request, response, &IRelationCommandService::PinDialog);
}

grpc::Status ChatRelationInternalGrpcService::FilterFriendUids(grpc::ServerContext* context,
                                                               const chatinternal::JsonPayloadRequest* request,
                                                               chatinternal::JsonPayloadResponse* response)
{
    if (const auto auth = Authorize(context,
                                    _access_mode == RelationGrpcAccessMode::QueryOnly ? RelationGrpcCaller::Moments
                                                                                      : RelationGrpcCaller::Chat);
        !auth.ok())
    {
        return auth;
    }
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

grpc::Status ChatRelationInternalGrpcService::RejectRestrictedCommand(chatinternal::JsonPayloadResponse* response) const
{
    if (response != nullptr)
    {
        response->set_error(ErrorCodes::RPCFailed);
        response->set_payload_json(relation_internal_grpc_modules::DefaultPayloadJson());
    }
    return grpc::Status(grpc::StatusCode::PERMISSION_DENIED, relation_internal_grpc_modules::CommandDisabledMessage());
}

grpc::Status ChatRelationInternalGrpcService::Authorize(grpc::ServerContext* context, RelationGrpcCaller caller) const
{
    if (_auth_mode == RelationGrpcAuthMode::DisabledForTests)
    {
        return grpc::Status::OK;
    }
    if (context == nullptr)
    {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "relation service authentication failed");
    }

    memolog::BindGrpcTraceContext(context);
    const std::string* expected_token = &_auth_tokens.chat;
    if (_access_mode == RelationGrpcAccessMode::QueryOnly)
    {
        switch (caller)
        {
            case RelationGrpcCaller::Chat:
                expected_token = &_auth_tokens.chat;
                break;
            case RelationGrpcCaller::Call:
                expected_token = &_auth_tokens.call;
                break;
            case RelationGrpcCaller::Moments:
                expected_token = &_auth_tokens.moments;
                break;
        }
    }

    if (!memochat::auth::HasValidRelationGrpcAuth(*context, *expected_token))
    {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "relation service authentication failed");
    }
    return grpc::Status::OK;
}

grpc::Status ChatRelationInternalGrpcService::AuthorizeHealth(grpc::ServerContext* context) const
{
    if (_auth_mode == RelationGrpcAuthMode::DisabledForTests)
    {
        return grpc::Status::OK;
    }
    if (context == nullptr)
    {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "relation service authentication failed");
    }

    memolog::BindGrpcTraceContext(context);
    if (_access_mode == RelationGrpcAccessMode::ReadWrite)
    {
        return Authorize(context, RelationGrpcCaller::Chat);
    }

    // A query-only health endpoint is shared by the independently scoped
    // Chat/Call/Moments probes. Accept exactly one of those configured role
    // tokens; no token is accepted more broadly than the configured allowlist.
    if (memochat::auth::HasValidRelationGrpcAuth(*context, _auth_tokens.chat) ||
        memochat::auth::HasValidRelationGrpcAuth(*context, _auth_tokens.call) ||
        memochat::auth::HasValidRelationGrpcAuth(*context, _auth_tokens.moments))
    {
        return grpc::Status::OK;
    }
    return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "relation service authentication failed");
}

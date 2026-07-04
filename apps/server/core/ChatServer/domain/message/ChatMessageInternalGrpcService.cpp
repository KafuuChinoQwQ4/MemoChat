#include "ChatMessageInternalGrpcService.hpp"

#include "const.hpp"
#include "json/GlazeCompat.hpp"

import memochat.chat.message_internal_grpc_service_algorithms;

namespace message_internal_grpc_modules = memochat::chat::message_internal_grpc_service::modules;

namespace
{
MessageCommandRequest BuildCommandRequest(const chatinternal::JsonPayloadRequest& request)
{
    MessageCommandRequest command_request;
    command_request.request_msg_id = message_internal_grpc_modules::TcpMessageId(request.tcp_msg_id());
    command_request.payload_json = request.payload_json();
    command_request.session_uid = request.session().uid();
    command_request.session_id = request.session().session_id();
    command_request.server_name = request.session().server_name();
    command_request.trace_id = request.session().trace_id();
    return command_request;
}

std::string CompactJson(const memochat::json::JsonValue& value)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, value);
}
} // namespace

ChatMessageInternalGrpcService::ChatMessageInternalGrpcService(IPrivateMessageCommandService* private_message_service,
                                                               IGroupMessageCommandService* group_message_service)
    : _private_message_service(private_message_service)
    , _group_message_service(group_message_service)
{
}

grpc::Status ChatMessageInternalGrpcService::SendTextChatMessage(grpc::ServerContext*,
                                                                 const chatinternal::JsonPayloadRequest* request,
                                                                 chatinternal::JsonPayloadResponse* response)
{
    return BuildPrivateCommandResponse(request, response, &IPrivateMessageCommandService::TextChatMessage);
}

grpc::Status ChatMessageInternalGrpcService::ForwardPrivateMessage(grpc::ServerContext*,
                                                                   const chatinternal::JsonPayloadRequest* request,
                                                                   chatinternal::JsonPayloadResponse* response)
{
    return BuildPrivateCommandResponse(request, response, &IPrivateMessageCommandService::ForwardPrivateMessage);
}

grpc::Status ChatMessageInternalGrpcService::PrivateReadAck(grpc::ServerContext*,
                                                            const chatinternal::JsonPayloadRequest* request,
                                                            chatinternal::JsonPayloadResponse* response)
{
    return BuildPrivateCommandResponse(request, response, &IPrivateMessageCommandService::PrivateReadAck);
}

grpc::Status ChatMessageInternalGrpcService::EditPrivateMessage(grpc::ServerContext*,
                                                                const chatinternal::JsonPayloadRequest* request,
                                                                chatinternal::JsonPayloadResponse* response)
{
    return BuildPrivateCommandResponse(request, response, &IPrivateMessageCommandService::EditPrivateMessage);
}

grpc::Status ChatMessageInternalGrpcService::RevokePrivateMessage(grpc::ServerContext*,
                                                                  const chatinternal::JsonPayloadRequest* request,
                                                                  chatinternal::JsonPayloadResponse* response)
{
    return BuildPrivateCommandResponse(request, response, &IPrivateMessageCommandService::RevokePrivateMessage);
}

grpc::Status ChatMessageInternalGrpcService::PrivateHistory(grpc::ServerContext*,
                                                            const chatinternal::JsonPayloadRequest* request,
                                                            chatinternal::JsonPayloadResponse* response)
{
    return BuildPrivateCommandResponse(request, response, &IPrivateMessageCommandService::PrivateHistory);
}

grpc::Status ChatMessageInternalGrpcService::BuildGroupList(grpc::ServerContext*,
                                                            const chatinternal::BootstrapRequest* request,
                                                            chatinternal::BootstrapResponse* response)
{
    if (message_internal_grpc_modules::ShouldReportMissingRequestOrResponse(request != nullptr, response != nullptr))
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            message_internal_grpc_modules::MissingGroupListBootstrapRequestMessage());
    }
    if (message_internal_grpc_modules::ShouldReportMissingGroupService(_group_message_service != nullptr))
    {
        response->set_error(ErrorCodes::RPCFailed);
        response->set_payload_json(message_internal_grpc_modules::DefaultPayloadJson());
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                            message_internal_grpc_modules::GroupMessageCommandServiceNotConfiguredMessage());
    }

    memochat::json::JsonValue out(memochat::json::object_t{});
    _group_message_service->BuildGroupListJson(request->uid(), out);
    response->set_error(ErrorCodes::Success);
    response->set_payload_json(CompactJson(out));
    return grpc::Status::OK;
}

grpc::Status ChatMessageInternalGrpcService::CreateGroup(grpc::ServerContext*,
                                                         const chatinternal::JsonPayloadRequest* request,
                                                         chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::CreateGroup);
}

grpc::Status ChatMessageInternalGrpcService::GetGroupList(grpc::ServerContext*,
                                                          const chatinternal::JsonPayloadRequest* request,
                                                          chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::GetGroupList);
}

grpc::Status ChatMessageInternalGrpcService::InviteGroupMember(grpc::ServerContext*,
                                                               const chatinternal::JsonPayloadRequest* request,
                                                               chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::InviteGroupMember);
}

grpc::Status ChatMessageInternalGrpcService::ApplyJoinGroup(grpc::ServerContext*,
                                                            const chatinternal::JsonPayloadRequest* request,
                                                            chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::ApplyJoinGroup);
}

grpc::Status ChatMessageInternalGrpcService::ReviewGroupApply(grpc::ServerContext*,
                                                              const chatinternal::JsonPayloadRequest* request,
                                                              chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::ReviewGroupApply);
}

grpc::Status ChatMessageInternalGrpcService::GroupChatMessage(grpc::ServerContext*,
                                                              const chatinternal::JsonPayloadRequest* request,
                                                              chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::GroupChatMessage);
}

grpc::Status ChatMessageInternalGrpcService::GroupHistory(grpc::ServerContext*,
                                                          const chatinternal::JsonPayloadRequest* request,
                                                          chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::GroupHistory);
}

grpc::Status ChatMessageInternalGrpcService::EditGroupMessage(grpc::ServerContext*,
                                                              const chatinternal::JsonPayloadRequest* request,
                                                              chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::EditGroupMessage);
}

grpc::Status ChatMessageInternalGrpcService::RevokeGroupMessage(grpc::ServerContext*,
                                                                const chatinternal::JsonPayloadRequest* request,
                                                                chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::RevokeGroupMessage);
}

grpc::Status ChatMessageInternalGrpcService::ForwardGroupMessage(grpc::ServerContext*,
                                                                 const chatinternal::JsonPayloadRequest* request,
                                                                 chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::ForwardGroupMessage);
}

grpc::Status ChatMessageInternalGrpcService::GroupReadAck(grpc::ServerContext*,
                                                          const chatinternal::JsonPayloadRequest* request,
                                                          chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::GroupReadAck);
}

grpc::Status ChatMessageInternalGrpcService::UpdateGroupAnnouncement(grpc::ServerContext*,
                                                                     const chatinternal::JsonPayloadRequest* request,
                                                                     chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::UpdateGroupAnnouncement);
}

grpc::Status ChatMessageInternalGrpcService::UpdateGroupIcon(grpc::ServerContext*,
                                                             const chatinternal::JsonPayloadRequest* request,
                                                             chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::UpdateGroupIcon);
}

grpc::Status ChatMessageInternalGrpcService::SetGroupAdmin(grpc::ServerContext*,
                                                           const chatinternal::JsonPayloadRequest* request,
                                                           chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::SetGroupAdmin);
}

grpc::Status ChatMessageInternalGrpcService::MuteGroupMember(grpc::ServerContext*,
                                                             const chatinternal::JsonPayloadRequest* request,
                                                             chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::MuteGroupMember);
}

grpc::Status ChatMessageInternalGrpcService::KickGroupMember(grpc::ServerContext*,
                                                             const chatinternal::JsonPayloadRequest* request,
                                                             chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::KickGroupMember);
}

grpc::Status ChatMessageInternalGrpcService::QuitGroup(grpc::ServerContext*,
                                                       const chatinternal::JsonPayloadRequest* request,
                                                       chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::QuitGroup);
}

grpc::Status ChatMessageInternalGrpcService::DissolveGroup(grpc::ServerContext*,
                                                           const chatinternal::JsonPayloadRequest* request,
                                                           chatinternal::JsonPayloadResponse* response)
{
    return BuildGroupCommandResponse(request, response, &IGroupMessageCommandService::DissolveGroup);
}

grpc::Status ChatMessageInternalGrpcService::BuildPrivateCommandResponse(
    const chatinternal::JsonPayloadRequest* request,
    chatinternal::JsonPayloadResponse* response,
    MessageCommandResult (IPrivateMessageCommandService::*handler)(const MessageCommandRequest&))
{
    if (message_internal_grpc_modules::ShouldReportMissingRequestOrResponse(request != nullptr, response != nullptr))
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            message_internal_grpc_modules::MissingPrivateMessageCommandRequestMessage());
    }
    if (message_internal_grpc_modules::ShouldReportMissingPrivateService(_private_message_service != nullptr))
    {
        response->set_error(ErrorCodes::RPCFailed);
        response->set_payload_json(message_internal_grpc_modules::DefaultPayloadJson());
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                            message_internal_grpc_modules::PrivateMessageCommandServiceNotConfiguredMessage());
    }

    const auto result = (_private_message_service->*handler)(BuildCommandRequest(*request));
    response->set_error(ErrorCodes::Success);
    response->set_tcp_msg_id(result.response_msg_id);
    response->set_payload_json(result.payload_json);
    return grpc::Status::OK;
}

grpc::Status ChatMessageInternalGrpcService::BuildGroupCommandResponse(
    const chatinternal::JsonPayloadRequest* request,
    chatinternal::JsonPayloadResponse* response,
    MessageCommandResult (IGroupMessageCommandService::*handler)(const MessageCommandRequest&))
{
    if (message_internal_grpc_modules::ShouldReportMissingRequestOrResponse(request != nullptr, response != nullptr))
    {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            message_internal_grpc_modules::MissingGroupMessageCommandRequestMessage());
    }
    if (message_internal_grpc_modules::ShouldReportMissingGroupService(_group_message_service != nullptr))
    {
        response->set_error(ErrorCodes::RPCFailed);
        response->set_payload_json(message_internal_grpc_modules::DefaultPayloadJson());
        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                            message_internal_grpc_modules::GroupMessageCommandServiceNotConfiguredMessage());
    }

    const auto result = (_group_message_service->*handler)(BuildCommandRequest(*request));
    response->set_error(ErrorCodes::Success);
    response->set_tcp_msg_id(result.response_msg_id);
    response->set_payload_json(result.payload_json);
    return grpc::Status::OK;
}

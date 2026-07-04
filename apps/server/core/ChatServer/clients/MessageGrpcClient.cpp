#include "MessageGrpcClient.hpp"

#include "const.hpp"
#include "json/GlazeCompat.hpp"
#include "logging/GrpcTrace.hpp"

#include <utility>

import memochat.chat.message_grpc_client_algorithms;

namespace message_grpc_modules = memochat::chat::message_grpc_client::modules;

namespace
{
const char* PrivateCommandMethodName(MessageGrpcClient::PrivateCommandRpc rpc)
{
    return message_grpc_modules::PrivateCommandMethodName(static_cast<int>(rpc));
}

const char* GroupCommandMethodName(MessageGrpcClient::GroupCommandRpc rpc)
{
    return message_grpc_modules::GroupCommandMethodName(static_cast<int>(rpc));
}

void MarkRemoteError(memochat::json::JsonValue& out, const char* method_name, const std::string& error, int status_code)
{
    out[message_grpc_modules::RemoteMethodField()] = method_name;
    out[message_grpc_modules::RemoteErrorField()] = error;
    out[message_grpc_modules::RemoteStatusCodeField()] = status_code;
}

std::string CompactJson(const memochat::json::JsonValue& value)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, value);
}

std::string ErrorPayload(const char* method_name, const std::string& error, int status_code)
{
    memochat::json::JsonValue payload(memochat::json::object_t{});
    payload[message_grpc_modules::ErrorField()] = ErrorCodes::RPCFailed;
    payload[message_grpc_modules::RemoteMethodField()] = method_name;
    payload[message_grpc_modules::RemoteErrorField()] = error;
    payload[message_grpc_modules::RemoteStatusCodeField()] = status_code;
    return CompactJson(payload);
}

void MergePayload(memochat::json::JsonValue& out, const std::string& payload_json, const char* method_name)
{
    if (!message_grpc_modules::ShouldMergePayload(payload_json.empty()))
    {
        return;
    }

    memochat::json::JsonValue payload(memochat::json::object_t{});
    const bool parse_ok = memochat::json::reader_parse(payload_json, payload);
    if (message_grpc_modules::ShouldReportInvalidPayload(parse_ok, payload.isObject()))
    {
        MarkRemoteError(out, method_name, message_grpc_modules::InvalidPayloadJsonMessage(), ErrorCodes::RPCFailed);
        return;
    }

    for (const auto& key : memochat::json::getMemberNames(payload))
    {
        out[key] = payload.get(key).asValue();
    }
}

chatinternal::JsonPayloadRequest BuildGrpcCommandRequest(const MessageCommandRequest& request)
{
    chatinternal::JsonPayloadRequest grpc_request;
    grpc_request.set_tcp_msg_id(request.request_msg_id);
    grpc_request.set_payload_json(request.payload_json);
    grpc_request.mutable_session()->set_uid(request.session_uid);
    grpc_request.mutable_session()->set_session_id(request.session_id);
    grpc_request.mutable_session()->set_server_name(request.server_name);
    grpc_request.mutable_session()->set_trace_id(request.trace_id);
    return grpc_request;
}
} // namespace

MessageGrpcClient::MessageGrpcClient(const std::string& endpoint, std::chrono::milliseconds timeout)
    : MessageGrpcClient(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()), timeout)
{
}

MessageGrpcClient::MessageGrpcClient(std::shared_ptr<grpc::Channel> channel, std::chrono::milliseconds timeout)
    : _timeout(timeout)
{
    if (channel)
    {
        _private_stub = chatinternal::ChatPrivateMessageInternalService::NewStub(channel);
        _group_stub = chatinternal::ChatGroupMessageInternalService::NewStub(std::move(channel));
    }
}

MessageCommandResult MessageGrpcClient::TextChatMessage(const MessageCommandRequest& request)
{
    return CallPrivateCommand(PrivateCommandRpc::SendTextChatMessage, request);
}

MessageCommandResult MessageGrpcClient::ForwardPrivateMessage(const MessageCommandRequest& request)
{
    return CallPrivateCommand(PrivateCommandRpc::ForwardPrivateMessage, request);
}

MessageCommandResult MessageGrpcClient::PrivateReadAck(const MessageCommandRequest& request)
{
    return CallPrivateCommand(PrivateCommandRpc::PrivateReadAck, request);
}

MessageCommandResult MessageGrpcClient::EditPrivateMessage(const MessageCommandRequest& request)
{
    return CallPrivateCommand(PrivateCommandRpc::EditPrivateMessage, request);
}

MessageCommandResult MessageGrpcClient::RevokePrivateMessage(const MessageCommandRequest& request)
{
    return CallPrivateCommand(PrivateCommandRpc::RevokePrivateMessage, request);
}

MessageCommandResult MessageGrpcClient::PrivateHistory(const MessageCommandRequest& request)
{
    return CallPrivateCommand(PrivateCommandRpc::PrivateHistory, request);
}

void MessageGrpcClient::BuildGroupListJson(int uid, memochat::json::JsonValue& out)
{
    CallGroupQuery(uid, out);
}

MessageCommandResult MessageGrpcClient::CreateGroup(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::CreateGroup, request);
}

MessageCommandResult MessageGrpcClient::GetGroupList(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::GetGroupList, request);
}

MessageCommandResult MessageGrpcClient::InviteGroupMember(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::InviteGroupMember, request);
}

MessageCommandResult MessageGrpcClient::ApplyJoinGroup(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::ApplyJoinGroup, request);
}

MessageCommandResult MessageGrpcClient::ReviewGroupApply(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::ReviewGroupApply, request);
}

MessageCommandResult MessageGrpcClient::GroupChatMessage(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::GroupChatMessage, request);
}

MessageCommandResult MessageGrpcClient::GroupHistory(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::GroupHistory, request);
}

MessageCommandResult MessageGrpcClient::GroupReadAck(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::GroupReadAck, request);
}

MessageCommandResult MessageGrpcClient::EditGroupMessage(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::EditGroupMessage, request);
}

MessageCommandResult MessageGrpcClient::RevokeGroupMessage(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::RevokeGroupMessage, request);
}

MessageCommandResult MessageGrpcClient::ForwardGroupMessage(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::ForwardGroupMessage, request);
}

MessageCommandResult MessageGrpcClient::UpdateGroupAnnouncement(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::UpdateGroupAnnouncement, request);
}

MessageCommandResult MessageGrpcClient::UpdateGroupIcon(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::UpdateGroupIcon, request);
}

MessageCommandResult MessageGrpcClient::SetGroupAdmin(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::SetGroupAdmin, request);
}

MessageCommandResult MessageGrpcClient::MuteGroupMember(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::MuteGroupMember, request);
}

MessageCommandResult MessageGrpcClient::KickGroupMember(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::KickGroupMember, request);
}

MessageCommandResult MessageGrpcClient::QuitGroup(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::QuitGroup, request);
}

MessageCommandResult MessageGrpcClient::DissolveGroup(const MessageCommandRequest& request)
{
    return CallGroupCommand(GroupCommandRpc::DissolveGroup, request);
}

void MessageGrpcClient::CallGroupQuery(int uid, memochat::json::JsonValue& out)
{
    const char* method_name = message_grpc_modules::BuildGroupListMethod();
    if (message_grpc_modules::ShouldReportMissingStub(_group_stub != nullptr))
    {
        MarkRemoteError(out, method_name, message_grpc_modules::GroupStubNotConfiguredMessage(), ErrorCodes::RPCFailed);
        return;
    }

    chatinternal::BootstrapRequest request;
    request.set_uid(uid);

    chatinternal::BootstrapResponse response;
    grpc::ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    context.set_deadline(std::chrono::system_clock::now() + _timeout);

    const auto status = _group_stub->BuildGroupList(&context, request, &response);
    if (message_grpc_modules::ShouldReportStatusError(status.ok()))
    {
        MarkRemoteError(out, method_name, status.error_message(), static_cast<int>(status.error_code()));
        return;
    }
    if (message_grpc_modules::ShouldReportBusinessError(response.error(), ErrorCodes::Success))
    {
        out[message_grpc_modules::RemoteErrorCodeField()] = response.error();
    }
    MergePayload(out, response.payload_json(), method_name);
}

MessageCommandResult MessageGrpcClient::CallPrivateCommand(PrivateCommandRpc rpc, const MessageCommandRequest& request)
{
    const char* method_name = PrivateCommandMethodName(rpc);
    if (message_grpc_modules::ShouldReportMissingStub(_private_stub != nullptr))
    {
        return {
            request.request_msg_id,
            ErrorPayload(method_name, message_grpc_modules::PrivateStubNotConfiguredMessage(), ErrorCodes::RPCFailed)};
    }

    auto grpc_request = BuildGrpcCommandRequest(request);
    chatinternal::JsonPayloadResponse response;
    grpc::ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    context.set_deadline(std::chrono::system_clock::now() + _timeout);

    grpc::Status status;
    switch (rpc)
    {
        case PrivateCommandRpc::SendTextChatMessage:
            status = _private_stub->SendTextChatMessage(&context, grpc_request, &response);
            break;
        case PrivateCommandRpc::ForwardPrivateMessage:
            status = _private_stub->ForwardPrivateMessage(&context, grpc_request, &response);
            break;
        case PrivateCommandRpc::PrivateReadAck:
            status = _private_stub->PrivateReadAck(&context, grpc_request, &response);
            break;
        case PrivateCommandRpc::EditPrivateMessage:
            status = _private_stub->EditPrivateMessage(&context, grpc_request, &response);
            break;
        case PrivateCommandRpc::RevokePrivateMessage:
            status = _private_stub->RevokePrivateMessage(&context, grpc_request, &response);
            break;
        case PrivateCommandRpc::PrivateHistory:
            status = _private_stub->PrivateHistory(&context, grpc_request, &response);
            break;
    }

    if (message_grpc_modules::ShouldReportStatusError(status.ok()))
    {
        return {request.request_msg_id,
                ErrorPayload(method_name, status.error_message(), static_cast<int>(status.error_code()))};
    }
    if (message_grpc_modules::ShouldReportBusinessError(response.error(), ErrorCodes::Success))
    {
        return {static_cast<short>(response.tcp_msg_id()),
                ErrorPayload(method_name, message_grpc_modules::PrivateBusinessErrorMessage(), response.error())};
    }
    return {static_cast<short>(response.tcp_msg_id()), response.payload_json()};
}

MessageCommandResult MessageGrpcClient::CallGroupCommand(GroupCommandRpc rpc, const MessageCommandRequest& request)
{
    const char* method_name = GroupCommandMethodName(rpc);
    if (message_grpc_modules::ShouldReportMissingStub(_group_stub != nullptr))
    {
        return {
            request.request_msg_id,
            ErrorPayload(method_name, message_grpc_modules::GroupStubNotConfiguredMessage(), ErrorCodes::RPCFailed)};
    }

    auto grpc_request = BuildGrpcCommandRequest(request);
    chatinternal::JsonPayloadResponse response;
    grpc::ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    context.set_deadline(std::chrono::system_clock::now() + _timeout);

    grpc::Status status;
    switch (rpc)
    {
        case GroupCommandRpc::CreateGroup:
            status = _group_stub->CreateGroup(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::GetGroupList:
            status = _group_stub->GetGroupList(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::InviteGroupMember:
            status = _group_stub->InviteGroupMember(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::ApplyJoinGroup:
            status = _group_stub->ApplyJoinGroup(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::ReviewGroupApply:
            status = _group_stub->ReviewGroupApply(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::GroupChatMessage:
            status = _group_stub->GroupChatMessage(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::GroupHistory:
            status = _group_stub->GroupHistory(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::EditGroupMessage:
            status = _group_stub->EditGroupMessage(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::RevokeGroupMessage:
            status = _group_stub->RevokeGroupMessage(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::ForwardGroupMessage:
            status = _group_stub->ForwardGroupMessage(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::GroupReadAck:
            status = _group_stub->GroupReadAck(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::UpdateGroupAnnouncement:
            status = _group_stub->UpdateGroupAnnouncement(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::UpdateGroupIcon:
            status = _group_stub->UpdateGroupIcon(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::SetGroupAdmin:
            status = _group_stub->SetGroupAdmin(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::MuteGroupMember:
            status = _group_stub->MuteGroupMember(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::KickGroupMember:
            status = _group_stub->KickGroupMember(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::QuitGroup:
            status = _group_stub->QuitGroup(&context, grpc_request, &response);
            break;
        case GroupCommandRpc::DissolveGroup:
            status = _group_stub->DissolveGroup(&context, grpc_request, &response);
            break;
    }

    if (message_grpc_modules::ShouldReportStatusError(status.ok()))
    {
        return {request.request_msg_id,
                ErrorPayload(method_name, status.error_message(), static_cast<int>(status.error_code()))};
    }
    if (message_grpc_modules::ShouldReportBusinessError(response.error(), ErrorCodes::Success))
    {
        return {static_cast<short>(response.tcp_msg_id()),
                ErrorPayload(method_name, message_grpc_modules::GroupBusinessErrorMessage(), response.error())};
    }
    return {static_cast<short>(response.tcp_msg_id()), response.payload_json()};
}

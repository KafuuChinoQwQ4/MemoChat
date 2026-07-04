#include "RelationGrpcClient.hpp"

#include "const.hpp"
#include "logging/GrpcTrace.hpp"

#include <utility>

import memochat.chat.relation_grpc_client_algorithms;

namespace relation_grpc_modules = memochat::chat::relation_grpc_client::modules;

namespace
{
void MarkRemoteError(memochat::json::JsonValue& out, const char* method_name, const std::string& error, int status_code)
{
    out[relation_grpc_modules::ErrorField()] = ErrorCodes::RPCFailed;
    out[relation_grpc_modules::RemoteMethodField()] = method_name;
    out[relation_grpc_modules::RemoteErrorField()] = error;
    out[relation_grpc_modules::RemoteStatusCodeField()] = status_code;
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
    payload[relation_grpc_modules::ErrorField()] = ErrorCodes::RPCFailed;
    payload[relation_grpc_modules::RemoteMethodField()] = method_name;
    payload[relation_grpc_modules::RemoteErrorField()] = error;
    payload[relation_grpc_modules::RemoteStatusCodeField()] = status_code;
    return CompactJson(payload);
}

void MergePayload(memochat::json::JsonValue& out, const std::string& payload_json, const char* method_name)
{
    if (!relation_grpc_modules::ShouldMergePayload(payload_json.empty()))
    {
        return;
    }

    memochat::json::JsonValue payload(memochat::json::object_t{});
    const bool parse_ok = memochat::json::reader_parse(payload_json, payload);
    if (relation_grpc_modules::ShouldReportInvalidPayload(parse_ok, payload.isObject()))
    {
        MarkRemoteError(out, method_name, relation_grpc_modules::InvalidPayloadJsonMessage(), ErrorCodes::RPCFailed);
        return;
    }

    for (const auto& key : memochat::json::getMemberNames(payload))
    {
        out[key] = payload.get(key).asValue();
    }
}

chatinternal::JsonPayloadRequest BuildGrpcCommandRequest(const RelationCommandRequest& request)
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

RelationGrpcClient::RelationGrpcClient(const std::string& endpoint, std::chrono::milliseconds timeout)
    : RelationGrpcClient(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()), timeout)
{
}

RelationGrpcClient::RelationGrpcClient(std::shared_ptr<grpc::Channel> channel, std::chrono::milliseconds timeout)
    : _timeout(timeout)
{
    if (channel)
    {
        _stub = chatinternal::ChatRelationInternalService::NewStub(std::move(channel));
    }
}

void RelationGrpcClient::AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out)
{
    CallQuery(QueryRpc::AppendRelationBootstrap, uid, out);
}

void RelationGrpcClient::BuildDialogListJson(int uid, memochat::json::JsonValue& out)
{
    CallQuery(QueryRpc::BuildDialogList, uid, out);
}

RelationCommandResult RelationGrpcClient::SearchUser(const RelationCommandRequest& request)
{
    return CallCommand(CommandRpc::SearchUser, request);
}

RelationCommandResult RelationGrpcClient::AddFriendApply(const RelationCommandRequest& request)
{
    return CallCommand(CommandRpc::AddFriendApply, request);
}

RelationCommandResult RelationGrpcClient::AuthFriendApply(const RelationCommandRequest& request)
{
    return CallCommand(CommandRpc::AuthFriendApply, request);
}

RelationCommandResult RelationGrpcClient::DeleteFriend(const RelationCommandRequest& request)
{
    return CallCommand(CommandRpc::DeleteFriend, request);
}

RelationCommandResult RelationGrpcClient::GetDialogList(const RelationCommandRequest& request)
{
    return CallCommand(CommandRpc::GetDialogList, request);
}

RelationCommandResult RelationGrpcClient::SyncDraft(const RelationCommandRequest& request)
{
    return CallCommand(CommandRpc::SyncDraft, request);
}

RelationCommandResult RelationGrpcClient::PinDialog(const RelationCommandRequest& request)
{
    return CallCommand(CommandRpc::PinDialog, request);
}

RelationCommandResult RelationGrpcClient::FilterFriendUids(const RelationCommandRequest& request)
{
    return CallCommand(CommandRpc::FilterFriendUids, request);
}

void RelationGrpcClient::CallQuery(QueryRpc rpc, int uid, memochat::json::JsonValue& out)
{
    const char* method_name = relation_grpc_modules::QueryMethodName(static_cast<int>(rpc));
    if (relation_grpc_modules::ShouldReportMissingStub(_stub != nullptr))
    {
        MarkRemoteError(out, method_name, relation_grpc_modules::StubNotConfiguredMessage(), ErrorCodes::RPCFailed);
        return;
    }

    chatinternal::BootstrapRequest request;
    request.set_uid(uid);

    chatinternal::BootstrapResponse response;
    grpc::ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    context.set_deadline(std::chrono::system_clock::now() + _timeout);

    grpc::Status status;
    switch (rpc)
    {
        case QueryRpc::AppendRelationBootstrap:
            status = _stub->AppendRelationBootstrap(&context, request, &response);
            break;
        case QueryRpc::BuildDialogList:
            status = _stub->BuildDialogList(&context, request, &response);
            break;
    }

    if (relation_grpc_modules::ShouldReportStatusError(status.ok()))
    {
        MarkRemoteError(out, method_name, status.error_message(), static_cast<int>(status.error_code()));
        return;
    }
    if (relation_grpc_modules::ShouldReportBusinessError(response.error(), ErrorCodes::Success))
    {
        out[relation_grpc_modules::RemoteErrorCodeField()] = response.error();
    }
    MergePayload(out, response.payload_json(), method_name);
}

RelationCommandResult RelationGrpcClient::CallCommand(CommandRpc rpc, const RelationCommandRequest& request)
{
    const char* method_name = relation_grpc_modules::CommandMethodName(static_cast<int>(rpc));
    if (relation_grpc_modules::ShouldReportMissingStub(_stub != nullptr))
    {
        return {request.request_msg_id,
                ErrorPayload(method_name, relation_grpc_modules::StubNotConfiguredMessage(), ErrorCodes::RPCFailed)};
    }

    auto grpc_request = BuildGrpcCommandRequest(request);
    chatinternal::JsonPayloadResponse response;
    grpc::ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    context.set_deadline(std::chrono::system_clock::now() + _timeout);

    grpc::Status status;
    switch (rpc)
    {
        case CommandRpc::SearchUser:
            status = _stub->SearchUser(&context, grpc_request, &response);
            break;
        case CommandRpc::AddFriendApply:
            status = _stub->AddFriendApply(&context, grpc_request, &response);
            break;
        case CommandRpc::AuthFriendApply:
            status = _stub->AuthFriendApply(&context, grpc_request, &response);
            break;
        case CommandRpc::DeleteFriend:
            status = _stub->DeleteFriend(&context, grpc_request, &response);
            break;
        case CommandRpc::GetDialogList:
            status = _stub->GetDialogList(&context, grpc_request, &response);
            break;
        case CommandRpc::SyncDraft:
            status = _stub->SyncDraft(&context, grpc_request, &response);
            break;
        case CommandRpc::PinDialog:
            status = _stub->PinDialog(&context, grpc_request, &response);
            break;
        case CommandRpc::FilterFriendUids:
            status = _stub->FilterFriendUids(&context, grpc_request, &response);
            break;
    }

    if (relation_grpc_modules::ShouldReportStatusError(status.ok()))
    {
        return {request.request_msg_id,
                ErrorPayload(method_name, status.error_message(), static_cast<int>(status.error_code()))};
    }
    if (relation_grpc_modules::ShouldReportBusinessError(response.error(), ErrorCodes::Success))
    {
        return {static_cast<short>(response.tcp_msg_id()),
                ErrorPayload(method_name, relation_grpc_modules::BusinessErrorMessage(), response.error())};
    }
    return {static_cast<short>(response.tcp_msg_id()), response.payload_json()};
}

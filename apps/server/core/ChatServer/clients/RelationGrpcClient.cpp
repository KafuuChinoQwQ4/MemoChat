#include "RelationGrpcClient.h"

#include "const.h"
#include "logging/GrpcTrace.h"

#include <utility>

namespace
{
const char* QueryMethodName(RelationGrpcClient::QueryRpc rpc)
{
    switch (rpc)
    {
        case RelationGrpcClient::QueryRpc::AppendRelationBootstrap:
            return "AppendRelationBootstrap";
        case RelationGrpcClient::QueryRpc::BuildDialogList:
            return "BuildDialogList";
    }
    return "unknown";
}

const char* CommandMethodName(RelationGrpcClient::CommandRpc rpc)
{
    switch (rpc)
    {
        case RelationGrpcClient::CommandRpc::SearchUser:
            return "SearchUser";
        case RelationGrpcClient::CommandRpc::AddFriendApply:
            return "AddFriendApply";
        case RelationGrpcClient::CommandRpc::AuthFriendApply:
            return "AuthFriendApply";
        case RelationGrpcClient::CommandRpc::DeleteFriend:
            return "DeleteFriend";
        case RelationGrpcClient::CommandRpc::GetDialogList:
            return "GetDialogList";
        case RelationGrpcClient::CommandRpc::SyncDraft:
            return "SyncDraft";
        case RelationGrpcClient::CommandRpc::PinDialog:
            return "PinDialog";
        case RelationGrpcClient::CommandRpc::FilterFriendUids:
            return "FilterFriendUids";
    }
    return "unknown";
}

void MarkRemoteError(memochat::json::JsonValue& out, const char* method_name, const std::string& error, int status_code)
{
    out["relation_remote_method"] = method_name;
    out["relation_remote_error"] = error;
    out["relation_remote_status_code"] = status_code;
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
    payload["error"] = ErrorCodes::RPCFailed;
    payload["relation_remote_method"] = method_name;
    payload["relation_remote_error"] = error;
    payload["relation_remote_status_code"] = status_code;
    return CompactJson(payload);
}

void MergePayload(memochat::json::JsonValue& out, const std::string& payload_json, const char* method_name)
{
    if (payload_json.empty())
    {
        return;
    }

    memochat::json::JsonValue payload(memochat::json::object_t{});
    if (!memochat::json::reader_parse(payload_json, payload) || !payload.isObject())
    {
        MarkRemoteError(out, method_name, "invalid relation payload json", ErrorCodes::RPCFailed);
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
    const char* method_name = QueryMethodName(rpc);
    if (!_stub)
    {
        MarkRemoteError(out, method_name, "relation grpc stub is not configured", ErrorCodes::RPCFailed);
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

    if (!status.ok())
    {
        MarkRemoteError(out, method_name, status.error_message(), static_cast<int>(status.error_code()));
        return;
    }
    if (response.error() != ErrorCodes::Success)
    {
        out["relation_remote_error_code"] = response.error();
    }
    MergePayload(out, response.payload_json(), method_name);
}

RelationCommandResult RelationGrpcClient::CallCommand(CommandRpc rpc, const RelationCommandRequest& request)
{
    const char* method_name = CommandMethodName(rpc);
    if (!_stub)
    {
        return {request.request_msg_id,
                ErrorPayload(method_name, "relation grpc stub is not configured", ErrorCodes::RPCFailed)};
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

    if (!status.ok())
    {
        return {request.request_msg_id,
                ErrorPayload(method_name, status.error_message(), static_cast<int>(status.error_code()))};
    }
    if (response.error() != ErrorCodes::Success)
    {
        return {static_cast<short>(response.tcp_msg_id()),
                ErrorPayload(method_name, "relation grpc business error", response.error())};
    }
    return {static_cast<short>(response.tcp_msg_id()), response.payload_json()};
}

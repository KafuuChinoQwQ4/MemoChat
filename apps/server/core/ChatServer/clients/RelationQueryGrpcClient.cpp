#include "RelationQueryGrpcClient.h"

#include "const.h"
#include "logging/GrpcTrace.h"

#include <utility>

namespace
{
void MarkRemoteError(memochat::json::JsonValue& out, const char* method_name, const std::string& error, int status_code)
{
    out["error"] = ErrorCodes::RPCFailed;
    out["relation_query_remote_method"] = method_name;
    out["relation_query_remote_error"] = error;
    out["relation_query_remote_status_code"] = status_code;
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
        MarkRemoteError(out, method_name, "invalid relation query payload json", ErrorCodes::RPCFailed);
        return;
    }

    for (const auto& key : memochat::json::getMemberNames(payload))
    {
        out[key] = payload.get(key).asValue();
    }
}
} // namespace

RelationQueryGrpcClient::RelationQueryGrpcClient(const std::string& endpoint, std::chrono::milliseconds timeout)
    : RelationQueryGrpcClient(grpc::CreateChannel(endpoint, grpc::InsecureChannelCredentials()), timeout)
{
}

RelationQueryGrpcClient::RelationQueryGrpcClient(std::shared_ptr<grpc::Channel> channel,
                                                 std::chrono::milliseconds timeout)
    : _timeout(timeout)
{
    if (channel)
    {
        _stub = chatinternal::ChatRelationInternalService::NewStub(std::move(channel));
    }
}

void RelationQueryGrpcClient::AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out)
{
    Call(QueryRpc::AppendRelationBootstrap, uid, out);
}

void RelationQueryGrpcClient::BuildDialogListJson(int uid, memochat::json::JsonValue& out)
{
    Call(QueryRpc::BuildDialogList, uid, out);
}

void RelationQueryGrpcClient::Call(QueryRpc rpc, int uid, memochat::json::JsonValue& out)
{
    const char* method_name = "unknown";
    switch (rpc)
    {
        case QueryRpc::AppendRelationBootstrap:
            method_name = "AppendRelationBootstrap";
            break;
        case QueryRpc::BuildDialogList:
            method_name = "BuildDialogList";
            break;
    }

    if (!_stub)
    {
        MarkRemoteError(out, method_name, "relation query grpc stub is not configured", ErrorCodes::RPCFailed);
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
        out["relation_query_remote_error_code"] = response.error();
    }

    MergePayload(out, response.payload_json(), method_name);
}

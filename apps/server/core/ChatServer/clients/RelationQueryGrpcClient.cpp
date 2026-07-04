#include "RelationQueryGrpcClient.hpp"

#include "const.hpp"
#include "logging/GrpcTrace.hpp"

#include <utility>

import memochat.chat.relation_query_grpc_client_algorithms;

namespace relation_query_grpc_modules = memochat::chat::relation_query_grpc_client::modules;

namespace
{
void MarkRemoteError(memochat::json::JsonValue& out, const char* method_name, const std::string& error, int status_code)
{
    out[relation_query_grpc_modules::ErrorField()] = ErrorCodes::RPCFailed;
    out[relation_query_grpc_modules::RemoteMethodField()] = method_name;
    out[relation_query_grpc_modules::RemoteErrorField()] = error;
    out[relation_query_grpc_modules::RemoteStatusCodeField()] = status_code;
}

void MergePayload(memochat::json::JsonValue& out, const std::string& payload_json, const char* method_name)
{
    if (!relation_query_grpc_modules::ShouldMergePayload(payload_json.empty()))
    {
        return;
    }

    memochat::json::JsonValue payload(memochat::json::object_t{});
    const bool parse_ok = memochat::json::reader_parse(payload_json, payload);
    if (relation_query_grpc_modules::ShouldReportInvalidPayload(parse_ok, payload.isObject()))
    {
        MarkRemoteError(out,
                        method_name,
                        relation_query_grpc_modules::InvalidPayloadJsonMessage(),
                        ErrorCodes::RPCFailed);
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
    const char* method_name = relation_query_grpc_modules::QueryMethodName(static_cast<int>(rpc));

    if (relation_query_grpc_modules::ShouldReportMissingStub(_stub != nullptr))
    {
        MarkRemoteError(out,
                        method_name,
                        relation_query_grpc_modules::StubNotConfiguredMessage(),
                        ErrorCodes::RPCFailed);
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

    if (relation_query_grpc_modules::ShouldReportStatusError(status.ok()))
    {
        MarkRemoteError(out, method_name, status.error_message(), static_cast<int>(status.error_code()));
        return;
    }
    if (relation_query_grpc_modules::ShouldReportBusinessError(response.error(), ErrorCodes::Success))
    {
        out[relation_query_grpc_modules::RemoteErrorCodeField()] = response.error();
    }

    MergePayload(out, response.payload_json(), method_name);
}

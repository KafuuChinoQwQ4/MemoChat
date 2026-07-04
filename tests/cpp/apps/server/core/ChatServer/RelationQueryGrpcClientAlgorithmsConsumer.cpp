import memochat.chat.relation_query_grpc_client_algorithms;

namespace memochat::tests::chat::relation_query_grpc_client
{
const char* UnknownMethod()
{
    return memochat::chat::relation_query_grpc_client::modules::UnknownMethod();
}

const char* AppendRelationBootstrapMethod()
{
    return memochat::chat::relation_query_grpc_client::modules::AppendRelationBootstrapMethod();
}

const char* BuildDialogListMethod()
{
    return memochat::chat::relation_query_grpc_client::modules::BuildDialogListMethod();
}

const char* QueryMethodName(int rpc)
{
    return memochat::chat::relation_query_grpc_client::modules::QueryMethodName(rpc);
}

const char* ErrorField()
{
    return memochat::chat::relation_query_grpc_client::modules::ErrorField();
}

const char* RemoteMethodField()
{
    return memochat::chat::relation_query_grpc_client::modules::RemoteMethodField();
}

const char* RemoteErrorField()
{
    return memochat::chat::relation_query_grpc_client::modules::RemoteErrorField();
}

const char* RemoteStatusCodeField()
{
    return memochat::chat::relation_query_grpc_client::modules::RemoteStatusCodeField();
}

const char* RemoteErrorCodeField()
{
    return memochat::chat::relation_query_grpc_client::modules::RemoteErrorCodeField();
}

const char* InvalidPayloadJsonMessage()
{
    return memochat::chat::relation_query_grpc_client::modules::InvalidPayloadJsonMessage();
}

const char* StubNotConfiguredMessage()
{
    return memochat::chat::relation_query_grpc_client::modules::StubNotConfiguredMessage();
}

bool ShouldMergePayload(bool payload_empty)
{
    return memochat::chat::relation_query_grpc_client::modules::ShouldMergePayload(payload_empty);
}

bool ShouldReportInvalidPayload(bool parse_ok, bool is_object)
{
    return memochat::chat::relation_query_grpc_client::modules::ShouldReportInvalidPayload(parse_ok, is_object);
}

bool ShouldReportMissingStub(bool has_stub)
{
    return memochat::chat::relation_query_grpc_client::modules::ShouldReportMissingStub(has_stub);
}

bool ShouldReportStatusError(bool status_ok)
{
    return memochat::chat::relation_query_grpc_client::modules::ShouldReportStatusError(status_ok);
}

bool ShouldReportBusinessError(int response_error, int success_code)
{
    return memochat::chat::relation_query_grpc_client::modules::ShouldReportBusinessError(response_error, success_code);
}
} // namespace memochat::tests::chat::relation_query_grpc_client

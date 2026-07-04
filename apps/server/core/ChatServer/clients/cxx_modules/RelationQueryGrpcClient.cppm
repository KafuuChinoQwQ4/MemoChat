export module memochat.chat.relation_query_grpc_client_algorithms;

export namespace memochat::chat::relation_query_grpc_client::modules
{
const char* UnknownMethod()
{
    return "unknown";
}

const char* AppendRelationBootstrapMethod()
{
    return "AppendRelationBootstrap";
}

const char* BuildDialogListMethod()
{
    return "BuildDialogList";
}

const char* QueryMethodName(int rpc)
{
    switch (rpc)
    {
        case 0:
            return AppendRelationBootstrapMethod();
        case 1:
            return BuildDialogListMethod();
    }
    return UnknownMethod();
}

const char* ErrorField()
{
    return "error";
}

const char* RemoteMethodField()
{
    return "relation_query_remote_method";
}

const char* RemoteErrorField()
{
    return "relation_query_remote_error";
}

const char* RemoteStatusCodeField()
{
    return "relation_query_remote_status_code";
}

const char* RemoteErrorCodeField()
{
    return "relation_query_remote_error_code";
}

const char* InvalidPayloadJsonMessage()
{
    return "invalid relation query payload json";
}

const char* StubNotConfiguredMessage()
{
    return "relation query grpc stub is not configured";
}

bool ShouldMergePayload(bool payload_empty)
{
    return !payload_empty;
}

bool ShouldReportInvalidPayload(bool parse_ok, bool is_object)
{
    return !parse_ok || !is_object;
}

bool ShouldReportMissingStub(bool has_stub)
{
    return !has_stub;
}

bool ShouldReportStatusError(bool status_ok)
{
    return !status_ok;
}

bool ShouldReportBusinessError(int response_error, int success_code)
{
    return response_error != success_code;
}
} // namespace memochat::chat::relation_query_grpc_client::modules

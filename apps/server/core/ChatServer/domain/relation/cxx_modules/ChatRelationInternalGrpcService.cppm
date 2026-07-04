export module memochat.chat.relation_internal_grpc_service_algorithms;

export namespace memochat::chat::relation_internal_grpc_service::modules
{
const char* DefaultPayloadJson()
{
    return "{}";
}

const char* MissingBootstrapRequestMessage()
{
    return "missing bootstrap request or response";
}

const char* RelationServiceNotConfiguredMessage()
{
    return "relation service is not configured";
}

const char* UidMustBePositiveMessage()
{
    return "uid must be positive";
}

const char* MissingRelationCommandRequestMessage()
{
    return "missing relation command request or response";
}

bool ShouldReportMissingRequestOrResponse(bool has_request, bool has_response)
{
    return !has_request || !has_response;
}

bool ShouldReportMissingRelationService(bool has_service)
{
    return !has_service;
}

bool ShouldReportInvalidUid(int uid)
{
    return uid <= 0;
}

short TcpMessageId(int value)
{
    return static_cast<short>(value);
}
} // namespace memochat::chat::relation_internal_grpc_service::modules

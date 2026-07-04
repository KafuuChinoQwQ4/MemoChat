import memochat.chat.relation_internal_grpc_service_algorithms;

namespace memochat::tests::chat::relation_internal_grpc_service
{
const char* DefaultPayloadJson()
{
    return memochat::chat::relation_internal_grpc_service::modules::DefaultPayloadJson();
}

const char* MissingBootstrapRequestMessage()
{
    return memochat::chat::relation_internal_grpc_service::modules::MissingBootstrapRequestMessage();
}

const char* RelationServiceNotConfiguredMessage()
{
    return memochat::chat::relation_internal_grpc_service::modules::RelationServiceNotConfiguredMessage();
}

const char* UidMustBePositiveMessage()
{
    return memochat::chat::relation_internal_grpc_service::modules::UidMustBePositiveMessage();
}

const char* MissingRelationCommandRequestMessage()
{
    return memochat::chat::relation_internal_grpc_service::modules::MissingRelationCommandRequestMessage();
}

bool ShouldReportMissingRequestOrResponse(bool has_request, bool has_response)
{
    return memochat::chat::relation_internal_grpc_service::modules::ShouldReportMissingRequestOrResponse(has_request,
                                                                                                         has_response);
}

bool ShouldReportMissingRelationService(bool has_service)
{
    return memochat::chat::relation_internal_grpc_service::modules::ShouldReportMissingRelationService(has_service);
}

bool ShouldReportInvalidUid(int uid)
{
    return memochat::chat::relation_internal_grpc_service::modules::ShouldReportInvalidUid(uid);
}

short TcpMessageId(int value)
{
    return memochat::chat::relation_internal_grpc_service::modules::TcpMessageId(value);
}
} // namespace memochat::tests::chat::relation_internal_grpc_service

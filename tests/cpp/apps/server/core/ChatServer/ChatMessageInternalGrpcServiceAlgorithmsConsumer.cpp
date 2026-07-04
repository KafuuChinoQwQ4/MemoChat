import memochat.chat.message_internal_grpc_service_algorithms;

namespace memochat::tests::chat::message_internal_grpc_service
{
const char* DefaultPayloadJson()
{
    return memochat::chat::message_internal_grpc_service::modules::DefaultPayloadJson();
}

const char* MissingGroupListBootstrapRequestMessage()
{
    return memochat::chat::message_internal_grpc_service::modules::MissingGroupListBootstrapRequestMessage();
}

const char* GroupMessageCommandServiceNotConfiguredMessage()
{
    return memochat::chat::message_internal_grpc_service::modules::GroupMessageCommandServiceNotConfiguredMessage();
}

const char* MissingPrivateMessageCommandRequestMessage()
{
    return memochat::chat::message_internal_grpc_service::modules::MissingPrivateMessageCommandRequestMessage();
}

const char* PrivateMessageCommandServiceNotConfiguredMessage()
{
    return memochat::chat::message_internal_grpc_service::modules::PrivateMessageCommandServiceNotConfiguredMessage();
}

const char* MissingGroupMessageCommandRequestMessage()
{
    return memochat::chat::message_internal_grpc_service::modules::MissingGroupMessageCommandRequestMessage();
}

bool ShouldReportMissingRequestOrResponse(bool has_request, bool has_response)
{
    return memochat::chat::message_internal_grpc_service::modules::ShouldReportMissingRequestOrResponse(has_request,
                                                                                                        has_response);
}

bool ShouldReportMissingPrivateService(bool has_service)
{
    return memochat::chat::message_internal_grpc_service::modules::ShouldReportMissingPrivateService(has_service);
}

bool ShouldReportMissingGroupService(bool has_service)
{
    return memochat::chat::message_internal_grpc_service::modules::ShouldReportMissingGroupService(has_service);
}

short TcpMessageId(int value)
{
    return memochat::chat::message_internal_grpc_service::modules::TcpMessageId(value);
}
} // namespace memochat::tests::chat::message_internal_grpc_service

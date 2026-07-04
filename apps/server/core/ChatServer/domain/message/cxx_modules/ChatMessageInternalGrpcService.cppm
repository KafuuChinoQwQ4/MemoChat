export module memochat.chat.message_internal_grpc_service_algorithms;

export namespace memochat::chat::message_internal_grpc_service::modules
{
const char* DefaultPayloadJson()
{
    return "{}";
}

const char* MissingGroupListBootstrapRequestMessage()
{
    return "missing group list bootstrap request or response";
}

const char* GroupMessageCommandServiceNotConfiguredMessage()
{
    return "group message command service is not configured";
}

const char* MissingPrivateMessageCommandRequestMessage()
{
    return "missing private message command request or response";
}

const char* PrivateMessageCommandServiceNotConfiguredMessage()
{
    return "private message command service is not configured";
}

const char* MissingGroupMessageCommandRequestMessage()
{
    return "missing group message command request or response";
}

bool ShouldReportMissingRequestOrResponse(bool has_request, bool has_response)
{
    return !has_request || !has_response;
}

bool ShouldReportMissingPrivateService(bool has_service)
{
    return !has_service;
}

bool ShouldReportMissingGroupService(bool has_service)
{
    return !has_service;
}

short TcpMessageId(int value)
{
    return static_cast<short>(value);
}
} // namespace memochat::chat::message_internal_grpc_service::modules

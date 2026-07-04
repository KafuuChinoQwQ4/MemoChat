import memochat.chat.message_grpc_client_algorithms;

namespace memochat::tests::chat::message_grpc_client
{
const char* UnknownMethod()
{
    return memochat::chat::message_grpc_client::modules::UnknownMethod();
}

const char* BuildGroupListMethod()
{
    return memochat::chat::message_grpc_client::modules::BuildGroupListMethod();
}

const char* PrivateCommandMethodName(int rpc)
{
    return memochat::chat::message_grpc_client::modules::PrivateCommandMethodName(rpc);
}

const char* GroupCommandMethodName(int rpc)
{
    return memochat::chat::message_grpc_client::modules::GroupCommandMethodName(rpc);
}

const char* ErrorField()
{
    return memochat::chat::message_grpc_client::modules::ErrorField();
}

const char* RemoteMethodField()
{
    return memochat::chat::message_grpc_client::modules::RemoteMethodField();
}

const char* RemoteErrorField()
{
    return memochat::chat::message_grpc_client::modules::RemoteErrorField();
}

const char* RemoteStatusCodeField()
{
    return memochat::chat::message_grpc_client::modules::RemoteStatusCodeField();
}

const char* RemoteErrorCodeField()
{
    return memochat::chat::message_grpc_client::modules::RemoteErrorCodeField();
}

const char* InvalidPayloadJsonMessage()
{
    return memochat::chat::message_grpc_client::modules::InvalidPayloadJsonMessage();
}

const char* PrivateStubNotConfiguredMessage()
{
    return memochat::chat::message_grpc_client::modules::PrivateStubNotConfiguredMessage();
}

const char* GroupStubNotConfiguredMessage()
{
    return memochat::chat::message_grpc_client::modules::GroupStubNotConfiguredMessage();
}

const char* PrivateBusinessErrorMessage()
{
    return memochat::chat::message_grpc_client::modules::PrivateBusinessErrorMessage();
}

const char* GroupBusinessErrorMessage()
{
    return memochat::chat::message_grpc_client::modules::GroupBusinessErrorMessage();
}

bool ShouldMergePayload(bool payload_empty)
{
    return memochat::chat::message_grpc_client::modules::ShouldMergePayload(payload_empty);
}

bool ShouldReportInvalidPayload(bool parse_ok, bool is_object)
{
    return memochat::chat::message_grpc_client::modules::ShouldReportInvalidPayload(parse_ok, is_object);
}

bool ShouldReportMissingStub(bool has_stub)
{
    return memochat::chat::message_grpc_client::modules::ShouldReportMissingStub(has_stub);
}

bool ShouldReportStatusError(bool status_ok)
{
    return memochat::chat::message_grpc_client::modules::ShouldReportStatusError(status_ok);
}

bool ShouldReportBusinessError(int response_error, int success_code)
{
    return memochat::chat::message_grpc_client::modules::ShouldReportBusinessError(response_error, success_code);
}
} // namespace memochat::tests::chat::message_grpc_client

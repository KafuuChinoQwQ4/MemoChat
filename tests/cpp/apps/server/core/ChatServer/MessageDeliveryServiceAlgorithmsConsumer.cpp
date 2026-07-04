import memochat.chat.message_delivery_service_algorithms;

namespace memochat::tests::chat::message_delivery_service
{
bool ShouldSkipRecipient(int uid, int exclude_uid)
{
    return memochat::chat::message_delivery_service::modules::ShouldSkipRecipient(uid, exclude_uid);
}

bool IsDelivered(int error_code, int delivered, int success_code)
{
    return memochat::chat::message_delivery_service::modules::IsDelivered(error_code, delivered, success_code);
}

bool IsBatchRpcFailure(int error_code, int delivered, int success_code)
{
    return memochat::chat::message_delivery_service::modules::IsBatchRpcFailure(error_code, delivered, success_code);
}

bool ShouldSkipFallbackRetry(bool fallback_empty, bool fallback_equals_target)
{
    return memochat::chat::message_delivery_service::modules::ShouldSkipFallbackRetry(fallback_empty,
                                                                                      fallback_equals_target);
}

const char* OfflineNotifyTaskName()
{
    return memochat::chat::message_delivery_service::modules::OfflineNotifyTaskName();
}

const char* MessageDeliveryRetryTaskName()
{
    return memochat::chat::message_delivery_service::modules::MessageDeliveryRetryTaskName();
}

const char* OfflineReason()
{
    return memochat::chat::message_delivery_service::modules::OfflineReason();
}

const char* RpcFailedReason()
{
    return memochat::chat::message_delivery_service::modules::RpcFailedReason();
}

const char* InvalidPrivateNotifyReason()
{
    return memochat::chat::message_delivery_service::modules::InvalidPrivateNotifyReason();
}
} // namespace memochat::tests::chat::message_delivery_service

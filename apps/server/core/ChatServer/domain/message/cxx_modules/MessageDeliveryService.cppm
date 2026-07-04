export module memochat.chat.message_delivery_service_algorithms;

export namespace memochat::chat::message_delivery_service::modules
{
// Recipient dedup guard: non-positive uids and the excluded sender are dropped.
bool ShouldSkipRecipient(int uid, int exclude_uid)
{
    return uid <= 0 || uid == exclude_uid;
}

// A gRPC notify RPC counts as delivered only on the success error code with a
// positive delivered count.
bool IsDelivered(int error_code, int delivered, int success_code)
{
    return error_code == success_code && delivered > 0;
}

// A batch group notify RPC counts as a failure when the error code is not the
// success code or nothing was delivered.
bool IsBatchRpcFailure(int error_code, int delivered, int success_code)
{
    return error_code != success_code || delivered <= 0;
}

// After a primary remote notify fails, a fallback retry is skipped when the
// fallback server is empty or identical to the already-tried target server.
bool ShouldSkipFallbackRetry(bool fallback_empty, bool fallback_equals_target)
{
    return fallback_empty || fallback_equals_target;
}

// Task names used when enqueuing failed deliveries onto the task bus.
const char* OfflineNotifyTaskName()
{
    return "offline_notify";
}

const char* MessageDeliveryRetryTaskName()
{
    return "message_delivery_retry";
}

// Reason literals embedded into the delivery task payload.
const char* OfflineReason()
{
    return "offline";
}

const char* RpcFailedReason()
{
    return "rpc_failed";
}

const char* InvalidPrivateNotifyReason()
{
    return "invalid_private_notify";
}
} // namespace memochat::chat::message_delivery_service::modules

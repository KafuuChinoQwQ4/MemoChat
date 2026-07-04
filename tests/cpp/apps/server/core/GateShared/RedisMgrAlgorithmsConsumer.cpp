import memochat.gate.redis_mgr_algorithms;

namespace memochat::tests::gate::redis_mgr
{
int DefaultPoolSize()
{
    return memochat::gate::redis_mgr::modules::DefaultPoolSize();
}

int NormalizePoolSize(int configured_pool_size)
{
    return memochat::gate::redis_mgr::modules::NormalizePoolSize(configured_pool_size);
}

bool IsReplyType(int reply_type, int expected_type)
{
    return memochat::gate::redis_mgr::modules::IsReplyType(reply_type, expected_type);
}

bool IsStatusOk(int reply_type, int status_type, const char* status)
{
    return memochat::gate::redis_mgr::modules::IsStatusOk(reply_type, status_type, status);
}

bool IsPositiveIntegerReply(int reply_type, int integer_type, long long value)
{
    return memochat::gate::redis_mgr::modules::IsPositiveIntegerReply(reply_type, integer_type, value);
}

bool IsNonEmptyStringReply(int reply_type, int string_type, const char* value, unsigned long length)
{
    return memochat::gate::redis_mgr::modules::IsNonEmptyStringReply(reply_type, string_type, value, length);
}

bool ShouldSkipBatch(unsigned long item_count)
{
    return memochat::gate::redis_mgr::modules::ShouldSkipBatch(item_count);
}

bool IsRedisOk(int status, int ok_status)
{
    return memochat::gate::redis_mgr::modules::IsRedisOk(status, ok_status);
}

bool ShouldCollectPipelineReply(int status, int ok_status, bool has_reply)
{
    return memochat::gate::redis_mgr::modules::ShouldCollectPipelineReply(status, ok_status, has_reply);
}
} // namespace memochat::tests::gate::redis_mgr

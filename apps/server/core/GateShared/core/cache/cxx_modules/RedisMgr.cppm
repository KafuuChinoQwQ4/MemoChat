export module memochat.gate.redis_mgr_algorithms;

export namespace memochat::gate::redis_mgr::modules
{
int DefaultPoolSize()
{
    return 30;
}

int NormalizePoolSize(int configured_pool_size)
{
    return configured_pool_size > 0 ? configured_pool_size : DefaultPoolSize();
}

bool IsReplyType(int reply_type, int expected_type)
{
    return reply_type == expected_type;
}

bool IsStatusOk(int reply_type, int status_type, const char* status)
{
    return reply_type == status_type && status != nullptr &&
           ((status[0] == 'O' && status[1] == 'K' && status[2] == '\0') ||
            (status[0] == 'o' && status[1] == 'k' && status[2] == '\0'));
}

bool IsPositiveIntegerReply(int reply_type, int integer_type, long long value)
{
    return reply_type == integer_type && value > 0;
}

bool IsNonEmptyStringReply(int reply_type, int string_type, const char* value, unsigned long length)
{
    return reply_type == string_type && value != nullptr && length > 0;
}

bool ShouldSkipBatch(unsigned long item_count)
{
    return item_count == 0;
}

bool IsRedisOk(int status, int ok_status)
{
    return status == ok_status;
}

bool ShouldCollectPipelineReply(int status, int ok_status, bool has_reply)
{
    return IsRedisOk(status, ok_status) && has_reply;
}
} // namespace memochat::gate::redis_mgr::modules

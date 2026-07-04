export module memochat.varify.redis_algorithms;

export namespace memochat::varify::redis::modules
{
int DefaultRedisPort()
{
    return 6379;
}

int DefaultConnectionPoolSize()
{
    return 5;
}

bool HasPassword(char first_char)
{
    return first_char != '\0';
}

bool HasReply(bool reply_present)
{
    return reply_present;
}

bool IsExpectedReplyType(int actual_type, int expected_type)
{
    return actual_type == expected_type;
}

bool IsRedisContextErrored(int context_error)
{
    return context_error != 0;
}

bool ShouldDropPoolConnection(bool reply_present, int context_error)
{
    return !HasReply(reply_present) || IsRedisContextErrored(context_error);
}

long long RedisCounterError()
{
    return -1;
}

long long RedisMissingTtl()
{
    return -2;
}

bool IsFirstIncrement(long long count)
{
    return count == 1;
}
} // namespace memochat::varify::redis::modules

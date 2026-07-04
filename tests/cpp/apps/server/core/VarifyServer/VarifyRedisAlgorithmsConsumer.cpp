import memochat.varify.redis_algorithms;

namespace memochat::tests::varify::redis
{
int DefaultRedisPort()
{
    return memochat::varify::redis::modules::DefaultRedisPort();
}

int DefaultConnectionPoolSize()
{
    return memochat::varify::redis::modules::DefaultConnectionPoolSize();
}

bool HasPassword(char first_char)
{
    return memochat::varify::redis::modules::HasPassword(first_char);
}

bool HasReply(bool reply_present)
{
    return memochat::varify::redis::modules::HasReply(reply_present);
}

bool IsExpectedReplyType(int actual_type, int expected_type)
{
    return memochat::varify::redis::modules::IsExpectedReplyType(actual_type, expected_type);
}

bool ShouldDropPoolConnection(bool reply_present, int context_error)
{
    return memochat::varify::redis::modules::ShouldDropPoolConnection(reply_present, context_error);
}

long long RedisCounterError()
{
    return memochat::varify::redis::modules::RedisCounterError();
}

long long RedisMissingTtl()
{
    return memochat::varify::redis::modules::RedisMissingTtl();
}

bool IsFirstIncrement(long long count)
{
    return memochat::varify::redis::modules::IsFirstIncrement(count);
}
} // namespace memochat::tests::varify::redis

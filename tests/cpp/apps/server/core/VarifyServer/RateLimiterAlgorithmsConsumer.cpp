import memochat.varify.rate_limiter_algorithms;

namespace memochat::tests::varify::rate_limiter
{
const char* EmailRateLimitPrefix()
{
    return memochat::varify::rate_limiter::modules::EmailRateLimitPrefix();
}

const char* IpRateLimitPrefix()
{
    return memochat::varify::rate_limiter::modules::IpRateLimitPrefix();
}

bool IsRedisCounterError(long long count)
{
    return memochat::varify::rate_limiter::modules::IsRedisCounterError(count);
}

bool ShouldRateLimit(long long count, int max_requests)
{
    return memochat::varify::rate_limiter::modules::ShouldRateLimit(count, max_requests);
}

bool ShouldAllowRequest(long long count, int max_requests)
{
    return memochat::varify::rate_limiter::modules::ShouldAllowRequest(count, max_requests);
}
} // namespace memochat::tests::varify::rate_limiter

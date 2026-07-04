export module memochat.varify.rate_limiter_algorithms;

export namespace memochat::varify::rate_limiter::modules
{
const char* EmailRateLimitPrefix()
{
    return "varify_rl_email:";
}

const char* IpRateLimitPrefix()
{
    return "varify_rl_ip:";
}

bool IsRedisCounterError(long long count)
{
    return count < 0;
}

bool ShouldRateLimit(long long count, int max_requests)
{
    return count > max_requests;
}

bool ShouldAllowRequest(long long count, int max_requests)
{
    return !IsRedisCounterError(count) && !ShouldRateLimit(count, max_requests);
}
} // namespace memochat::varify::rate_limiter::modules

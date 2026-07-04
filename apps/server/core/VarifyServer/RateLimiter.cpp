#include "RateLimiter.hpp"
#include "VarifyRedisMgr.hpp"

#include <utility>

import memochat.varify.rate_limiter_algorithms;

namespace varifyservice
{
namespace rate_limiter_modules = memochat::varify::rate_limiter::modules;

RateLimiter::Result RateLimiter::Check(const std::string& key, int window_sec, int max_requests)
{
    int64_t count = VarifyRedisMgr::Instance().IncrWithExpire(key, window_sec);
    if (rate_limiter_modules::IsRedisCounterError(count))
    {
        return Result::Error;
    }
    if (rate_limiter_modules::ShouldRateLimit(count, max_requests))
    {
        return Result::RateLimited;
    }
    return Result::Allowed;
}

RateLimiter::Result RateLimiter::CheckEmail(const std::string& email, int window_sec, int max_requests)
{
    return Check(std::string(rate_limiter_modules::EmailRateLimitPrefix()) + email, window_sec, max_requests);
}

RateLimiter::Result RateLimiter::CheckIP(const std::string& ip, int window_sec, int max_requests)
{
    return Check(std::string(rate_limiter_modules::IpRateLimitPrefix()) + ip, window_sec, max_requests);
}

} // namespace varifyservice

#include "RateLimiter.h"
#include "VarifyRedisMgr.h"

#include <utility>

namespace varifyservice {

RateLimiter::Result RateLimiter::Check(const std::string& key, int window_sec, int max_requests) {
    int64_t count = VarifyRedisMgr::Instance().IncrWithExpire(key, window_sec);
    if (count < 0) {
        return Result::Error;
    }
    if (count > max_requests) {
        return Result::RateLimited;
    }
    return Result::Allowed;
}

RateLimiter::Result RateLimiter::CheckEmail(const std::string& email, int window_sec, int max_requests) {
    return Check("varify_rl_email:" + email, window_sec, max_requests);
}

RateLimiter::Result RateLimiter::CheckIP(const std::string& ip, int window_sec, int max_requests) {
    return Check("varify_rl_ip:" + ip, window_sec, max_requests);
}

} // namespace varifyservice

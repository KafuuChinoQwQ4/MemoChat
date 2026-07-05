#pragma once

#include "routing/GateRequest.hpp"

#include <string>
#include <string_view>

namespace gateauthsupport
{

enum class AuthRateLimitAction
{
    GetVarifyCode,
    Register,
    ResetPassword,
    AuthRefresh,
};

enum class AuthRateLimitBucket
{
    Email,
    Ip,
    Subject,
};

enum class AuthRateLimitRedisFailureBehavior
{
    FailClosed,
    BoundedFailOpen,
};

struct AuthLoginRateLimitResult
{
    bool rate_limited = false;
    bool redis_error = false;
    bool fallback_active = false;
    int retry_after_sec = 0;
};

const char* AuthRateLimitActionName(AuthRateLimitAction action);
const char* AuthRateLimitBucketName(AuthRateLimitBucket bucket);
std::string BuildAuthRateLimitKey(AuthRateLimitAction action, AuthRateLimitBucket bucket, std::string_view value);
AuthRateLimitRedisFailureBehavior ParseAuthRateLimitRedisFailureBehavior(std::string_view value);

AuthLoginRateLimitResult CheckAuthRequestRateLimit(AuthRateLimitAction action,
                                                   std::string_view subject,
                                                   const memochat::gate::routing::GateRequest& request);
AuthLoginRateLimitResult CheckLoginRateLimit(const std::string& email,
                                             const memochat::gate::routing::GateRequest& request);
AuthLoginRateLimitResult RecordLoginFailure(const std::string& email,
                                            const memochat::gate::routing::GateRequest& request);
void ClearLoginFailureCounters(const std::string& email, const memochat::gate::routing::GateRequest& request);

std::string ResolveLoginRateLimitClientIp(const memochat::gate::routing::GateRequest& request);
int LoginRateLimitWindowSec();
int LoginRateLimitEmailMaxFailures();
int LoginRateLimitIpMaxFailures();
bool LoginRateLimitEnabled();

} // namespace gateauthsupport

#pragma once

#include "routing/GateRequest.hpp"

#include <string>

namespace gateauthsupport
{

struct AuthLoginRateLimitResult
{
    bool rate_limited = false;
    bool redis_error = false;
    int retry_after_sec = 0;
};

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

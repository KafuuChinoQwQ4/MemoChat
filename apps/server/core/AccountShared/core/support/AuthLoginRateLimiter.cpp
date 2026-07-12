#include "AuthLoginRateLimiter.hpp"
#include "AuthLocalFallbackCounterStore.hpp"

#include "common_lua_scripts.hpp"
#include "ConfigMgr.hpp"
#include "RedisMgr.hpp"
#include "logging/Logger.hpp"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace gateauthsupport
{
namespace
{

constexpr const char* kSection = "AuthLoginRateLimit";
constexpr const char* kRequestSection = "AuthRequestRateLimit";
constexpr const char* kEmailPrefix = "auth_login_fail_email:";
constexpr const char* kIpPrefix = "auth_login_fail_ip:";
constexpr const char* kRequestPrefix = "auth_req:";
constexpr const char* kFallbackPrefix = "local_fallback:";
constexpr int kDefaultWindowSec = 900;
constexpr int kDefaultEmailMaxFailures = 8;
constexpr int kDefaultIpMaxFailures = 40;
constexpr int kDefaultVerifyCodeWindowSec = 300;
constexpr int kDefaultVerifyCodeEmailMaxRequests = 5;
constexpr int kDefaultVerifyCodeIpMaxRequests = 30;
constexpr int kDefaultRegisterWindowSec = 900;
constexpr int kDefaultRegisterEmailMaxRequests = 3;
constexpr int kDefaultRegisterIpMaxRequests = 20;
constexpr int kDefaultResetPasswordWindowSec = 900;
constexpr int kDefaultResetPasswordEmailMaxRequests = 5;
constexpr int kDefaultResetPasswordIpMaxRequests = 30;
constexpr int kDefaultAuthRefreshWindowSec = 60;
constexpr int kDefaultAuthRefreshSubjectMaxRequests = 20;
constexpr int kDefaultAuthRefreshIpMaxRequests = 120;
constexpr std::size_t kLocalFallbackCounterCapacity = 50000;

std::string Trim(std::string value)
{
    auto not_space = [](unsigned char ch)
    {
        return !std::isspace(ch);
    };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string LowerAscii(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char ch)
                   {
                       return static_cast<char>(std::tolower(ch));
                   });
    return value;
}

bool EqualsIgnoreCase(std::string left, std::string right)
{
    return LowerAscii(std::move(left)) == LowerAscii(std::move(right));
}

std::string HeaderValue(const memochat::gate::routing::GateRequest& request, const std::string& name)
{
    for (const auto& [key, value] : request.headers)
    {
        if (EqualsIgnoreCase(key, name))
        {
            return value;
        }
    }
    return "";
}

std::string FirstForwardedForValue(const std::string& value)
{
    const auto comma = value.find(',');
    if (comma == std::string::npos)
    {
        return Trim(value);
    }
    return Trim(value.substr(0, comma));
}

bool IsFalseText(std::string value)
{
    value = LowerAscii(Trim(std::move(value)));
    return value == "0" || value == "false" || value == "off" || value == "no";
}

int ConfigInt(const std::string& section, const std::string& key, int default_value, int min_value, int max_value)
{
    const auto raw = ConfigMgr::Inst().GetValue(section, key);
    if (raw.empty())
    {
        return default_value;
    }
    int value = 0;
    const auto [ptr, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    if (ec != std::errc{} || ptr != raw.data() + raw.size())
    {
        return default_value;
    }
    return std::clamp(value, min_value, max_value);
}

int LoginConfigInt(const std::string& key, int default_value, int min_value, int max_value)
{
    return ConfigInt(kSection, key, default_value, min_value, max_value);
}

int RequestConfigInt(const std::string& key, int default_value, int min_value, int max_value)
{
    return ConfigInt(kRequestSection, key, default_value, min_value, max_value);
}

std::string ConfigValueOrEmpty(const std::string& section, const std::string& key)
{
    return ConfigMgr::Inst().GetValue(section, key);
}

std::string EmailKey(const std::string& email)
{
    return std::string(kEmailPrefix) + LowerAscii(Trim(email));
}

std::string IpKey(const std::string& ip)
{
    return std::string(kIpPrefix) + ip;
}

struct RedisCounter
{
    bool ok = false;
    bool capacity_exhausted = false;
    long long count = 0;
    int ttl_sec = 0;
};

struct RequestRateLimitDefaults
{
    int window_sec = 60;
    int subject_max_requests = 10;
    int ip_max_requests = 60;
};

RequestRateLimitDefaults DefaultsFor(AuthRateLimitAction action)
{
    switch (action)
    {
        case AuthRateLimitAction::GetVarifyCode:
            return {.window_sec = kDefaultVerifyCodeWindowSec,
                    .subject_max_requests = kDefaultVerifyCodeEmailMaxRequests,
                    .ip_max_requests = kDefaultVerifyCodeIpMaxRequests};
        case AuthRateLimitAction::Register:
            return {.window_sec = kDefaultRegisterWindowSec,
                    .subject_max_requests = kDefaultRegisterEmailMaxRequests,
                    .ip_max_requests = kDefaultRegisterIpMaxRequests};
        case AuthRateLimitAction::ResetPassword:
            return {.window_sec = kDefaultResetPasswordWindowSec,
                    .subject_max_requests = kDefaultResetPasswordEmailMaxRequests,
                    .ip_max_requests = kDefaultResetPasswordIpMaxRequests};
        case AuthRateLimitAction::AuthRefresh:
            return {.window_sec = kDefaultAuthRefreshWindowSec,
                    .subject_max_requests = kDefaultAuthRefreshSubjectMaxRequests,
                    .ip_max_requests = kDefaultAuthRefreshIpMaxRequests};
    }
    return {};
}

const char* ActionConfigPrefix(AuthRateLimitAction action)
{
    switch (action)
    {
        case AuthRateLimitAction::GetVarifyCode:
            return "GetVarifyCode";
        case AuthRateLimitAction::Register:
            return "Register";
        case AuthRateLimitAction::ResetPassword:
            return "ResetPassword";
        case AuthRateLimitAction::AuthRefresh:
            return "AuthRefresh";
    }
    return "Unknown";
}

AuthRateLimitBucket SubjectBucketFor(AuthRateLimitAction action)
{
    return action == AuthRateLimitAction::AuthRefresh ? AuthRateLimitBucket::Subject : AuthRateLimitBucket::Email;
}

std::string NormalizeBucketValue(AuthRateLimitBucket bucket, std::string_view value)
{
    std::string normalized = Trim(std::string(value));
    if (bucket == AuthRateLimitBucket::Email)
    {
        normalized = LowerAscii(std::move(normalized));
    }
    return normalized;
}

AuthLocalFallbackCounterStore& LocalFallbackCounterStore()
{
    static AuthLocalFallbackCounterStore store(kLocalFallbackCounterCapacity);
    return store;
}

RedisCounter IncrementLocalFallbackCounter(const std::string& key, int window_sec)
{
    const auto fallback =
        LocalFallbackCounterStore().Increment(key, window_sec, AuthLocalFallbackCounterStore::Clock::now());
    RedisCounter result;
    result.ttl_sec = window_sec;
    if (fallback.status == AuthLocalFallbackCounterStatus::CapacityExhausted)
    {
        result.capacity_exhausted = true;
        return result;
    }
    result.ok = true;
    result.count = fallback.count;
    result.ttl_sec = fallback.ttl_sec;
    return result;
}

int NormalizeRetryAfter(long long ttl, int window_sec)
{
    if (ttl > 0 && ttl <= 86400)
    {
        return static_cast<int>(ttl);
    }
    return window_sec;
}

RedisCounter ReadCounter(const std::string& key, int window_sec)
{
    RedisCounter result;
    auto* ctx = RedisMgr::GetInstance()->getRawConnection();
    if (ctx == nullptr)
    {
        return result;
    }

    auto release = [&]()
    {
        RedisMgr::GetInstance()->returnConnection(ctx);
    };

    redisReply* reply = static_cast<redisReply*>(redisCommand(ctx, "GET %s", key.c_str()));
    if (reply == nullptr)
    {
        release();
        return result;
    }
    if (reply->type == REDIS_REPLY_NIL)
    {
        freeReplyObject(reply);
        result.ok = true;
        result.count = 0;
        result.ttl_sec = 0;
        release();
        return result;
    }
    if (reply->type != REDIS_REPLY_STRING || reply->str == nullptr)
    {
        freeReplyObject(reply);
        release();
        return result;
    }
    int64_t count = 0;
    const auto [ptr, ec] = std::from_chars(reply->str, reply->str + reply->len, count);
    if (ec == std::errc{} && ptr == reply->str + reply->len)
    {
        result.count = count;
    }
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(ctx, "TTL %s", key.c_str()));
    result.ttl_sec = window_sec;
    if (reply != nullptr)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            result.ttl_sec = NormalizeRetryAfter(reply->integer, window_sec);
        }
        freeReplyObject(reply);
    }
    result.ok = true;
    release();
    return result;
}

RedisCounter IncrementCounter(const std::string& key, int window_sec)
{
    RedisCounter result;
    auto* ctx = RedisMgr::GetInstance()->getRawConnection();
    if (ctx == nullptr)
    {
        return result;
    }

    auto release = [&]()
    {
        RedisMgr::GetInstance()->returnConnection(ctx);
    };

    const std::string kLuaIncrExpire(memochat::common::lua_scripts::kincr_with_expire);

    redisReply* reply =
        static_cast<redisReply*>(redisCommand(ctx, "EVAL %s 1 %s %d", kLuaIncrExpire.c_str(), key.c_str(), window_sec));
    if (reply == nullptr)
    {
        release();
        return result;
    }
    if (reply->type != REDIS_REPLY_INTEGER)
    {
        freeReplyObject(reply);
        release();
        return result;
    }
    result.count = reply->integer;
    freeReplyObject(reply);

    reply = static_cast<redisReply*>(redisCommand(ctx, "TTL %s", key.c_str()));
    result.ttl_sec = window_sec;
    if (reply != nullptr)
    {
        if (reply->type == REDIS_REPLY_INTEGER)
        {
            result.ttl_sec = NormalizeRetryAfter(reply->integer, window_sec);
        }
        freeReplyObject(reply);
    }
    result.ok = true;
    release();
    return result;
}

AuthRateLimitRedisFailureBehavior ConfiguredRedisFailureBehavior()
{
    auto raw = ConfigValueOrEmpty(kRequestSection, "RedisFailureBehavior");
    if (raw.empty())
    {
        raw = ConfigValueOrEmpty(kSection, "RedisFailureBehavior");
    }
    return ParseAuthRateLimitRedisFailureBehavior(raw);
}

void ApplyRedisCounterError(AuthLoginRateLimitResult& result, const std::string& key, int max_requests, int window_sec)
{
    if (ConfiguredRedisFailureBehavior() != AuthRateLimitRedisFailureBehavior::BoundedFailOpen)
    {
        result.redis_error = true;
        return;
    }

    result.fallback_active = true;
    const auto fallback = IncrementLocalFallbackCounter(std::string(kFallbackPrefix) + key, window_sec);
    if (fallback.capacity_exhausted)
    {
        result.redis_error = true;
        result.rate_limited = true;
        result.retry_after_sec = std::max(result.retry_after_sec, window_sec);
        return;
    }
    if (fallback.count > max_requests)
    {
        result.rate_limited = true;
        result.retry_after_sec = std::max(result.retry_after_sec, NormalizeRetryAfter(fallback.ttl_sec, window_sec));
    }
}

void DeleteKeys(const std::vector<std::string>& keys)
{
    for (const auto& key : keys)
    {
        if (!key.empty())
        {
            RedisMgr::GetInstance()->Del(key);
        }
    }
}

void ApplyCounterLimit(AuthLoginRateLimitResult& result,
                       const RedisCounter& counter,
                       int max_failures,
                       int window_sec,
                       bool after_increment,
                       const std::string& key)
{
    if (!counter.ok)
    {
        ApplyRedisCounterError(result, key, max_failures, window_sec);
        return;
    }
    const bool limited = after_increment ? counter.count > max_failures : counter.count >= max_failures;
    if (limited)
    {
        result.rate_limited = true;
        result.retry_after_sec = std::max(result.retry_after_sec, NormalizeRetryAfter(counter.ttl_sec, window_sec));
    }
}

std::vector<std::string>
FailureKeysFor(const std::string& email, const memochat::gate::routing::GateRequest& request, bool include_ip)
{
    std::vector<std::string> keys;
    const auto normalized_email = LowerAscii(Trim(email));
    if (!normalized_email.empty())
    {
        keys.push_back(EmailKey(normalized_email));
    }
    if (include_ip)
    {
        const auto ip = ResolveLoginRateLimitClientIp(request);
        if (!ip.empty())
        {
            keys.push_back(IpKey(ip));
        }
    }
    return keys;
}

} // namespace

bool LoginRateLimitEnabled()
{
    return !IsFalseText(ConfigMgr::Inst().GetValue(kSection, "Enabled"));
}

int LoginRateLimitWindowSec()
{
    return LoginConfigInt("WindowSec", kDefaultWindowSec, 60, 86400);
}

int LoginRateLimitEmailMaxFailures()
{
    return LoginConfigInt("EmailMaxFailures", kDefaultEmailMaxFailures, 1, 1000);
}

int LoginRateLimitIpMaxFailures()
{
    return LoginConfigInt("IpMaxFailures", kDefaultIpMaxFailures, 1, 10000);
}

const char* AuthRateLimitActionName(AuthRateLimitAction action)
{
    switch (action)
    {
        case AuthRateLimitAction::GetVarifyCode:
            return "get_varifycode";
        case AuthRateLimitAction::Register:
            return "register";
        case AuthRateLimitAction::ResetPassword:
            return "reset_pwd";
        case AuthRateLimitAction::AuthRefresh:
            return "auth_refresh";
    }
    return "unknown";
}

const char* AuthRateLimitBucketName(AuthRateLimitBucket bucket)
{
    switch (bucket)
    {
        case AuthRateLimitBucket::Email:
            return "email";
        case AuthRateLimitBucket::Ip:
            return "ip";
        case AuthRateLimitBucket::Subject:
            return "subject";
    }
    return "unknown";
}

std::string BuildAuthRateLimitKey(AuthRateLimitAction action, AuthRateLimitBucket bucket, std::string_view value)
{
    return std::string(kRequestPrefix) + AuthRateLimitActionName(action) + ":" + AuthRateLimitBucketName(bucket) + ":" +
           NormalizeBucketValue(bucket, value);
}

AuthRateLimitRedisFailureBehavior ParseAuthRateLimitRedisFailureBehavior(std::string_view value)
{
    const auto normalized = LowerAscii(Trim(std::string(value)));
    if (normalized == "bounded_fail_open" || normalized == "bounded-open" || normalized == "bounded_open" ||
        normalized == "fail_open_bounded")
    {
        return AuthRateLimitRedisFailureBehavior::BoundedFailOpen;
    }
    return AuthRateLimitRedisFailureBehavior::FailClosed;
}

bool AuthRequestRateLimitEnabled(AuthRateLimitAction action)
{
    if (IsFalseText(ConfigMgr::Inst().GetValue(kRequestSection, "Enabled")))
    {
        return false;
    }
    return !IsFalseText(
        ConfigMgr::Inst().GetValue(kRequestSection, std::string(ActionConfigPrefix(action)) + "Enabled"));
}

int AuthRequestRateLimitWindowSec(AuthRateLimitAction action)
{
    const auto defaults = DefaultsFor(action);
    return RequestConfigInt(std::string(ActionConfigPrefix(action)) + "WindowSec", defaults.window_sec, 1, 86400);
}

int AuthRequestRateLimitSubjectMaxRequests(AuthRateLimitAction action)
{
    const auto defaults = DefaultsFor(action);
    return RequestConfigInt(
        std::string(ActionConfigPrefix(action)) +
            (SubjectBucketFor(action) == AuthRateLimitBucket::Email ? "EmailMaxRequests" : "SubjectMaxRequests"),
        defaults.subject_max_requests,
        1,
        100000);
}

int AuthRequestRateLimitIpMaxRequests(AuthRateLimitAction action)
{
    const auto defaults = DefaultsFor(action);
    return RequestConfigInt(std::string(ActionConfigPrefix(action)) + "IpMaxRequests",
                            defaults.ip_max_requests,
                            1,
                            100000);
}

std::string ResolveLoginRateLimitClientIp(const memochat::gate::routing::GateRequest& request)
{
    // Only trust X-Forwarded-For / X-Real-IP when the deployment explicitly opts in
    // via MEMOCHAT_TRUST_FORWARDED_FOR=1 — meaning the upstream proxy (Envoy) is
    // guaranteed to strip / override these headers before they reach GateServer.
    // Without this guard, clients can supply arbitrary X-Forwarded-For values to
    // rotate through unlimited IPs and bypass the IP-based rate limit bucket.
    const char* trust_xff = std::getenv("MEMOCHAT_TRUST_FORWARDED_FOR");
    if (trust_xff != nullptr && std::string_view(trust_xff) == "1")
    {
        const auto forwarded_for = FirstForwardedForValue(HeaderValue(request, "x-forwarded-for"));
        if (!forwarded_for.empty())
        {
            return forwarded_for;
        }
        const auto real_ip = Trim(HeaderValue(request, "x-real-ip"));
        if (!real_ip.empty())
        {
            return real_ip;
        }
    }
    return Trim(request.remote_address);
}

AuthLoginRateLimitResult CheckAuthRequestRateLimit(AuthRateLimitAction action,
                                                   std::string_view subject,
                                                   const memochat::gate::routing::GateRequest& request)
{
    AuthLoginRateLimitResult result;
    if (!AuthRequestRateLimitEnabled(action))
    {
        return result;
    }

    const int window_sec = AuthRequestRateLimitWindowSec(action);
    const int subject_max = AuthRequestRateLimitSubjectMaxRequests(action);
    const int ip_max = AuthRequestRateLimitIpMaxRequests(action);
    const auto subject_bucket = SubjectBucketFor(action);
    const auto normalized_subject = NormalizeBucketValue(subject_bucket, subject);
    if (!normalized_subject.empty())
    {
        const auto key = BuildAuthRateLimitKey(action, subject_bucket, normalized_subject);
        ApplyCounterLimit(result, IncrementCounter(key, window_sec), subject_max, window_sec, true, key);
    }

    const auto ip = ResolveLoginRateLimitClientIp(request);
    if (!ip.empty())
    {
        const auto key = BuildAuthRateLimitKey(action, AuthRateLimitBucket::Ip, ip);
        ApplyCounterLimit(result, IncrementCounter(key, window_sec), ip_max, window_sec, true, key);
    }
    return result;
}

AuthLoginRateLimitResult CheckLoginRateLimit(const std::string& email,
                                             const memochat::gate::routing::GateRequest& request)
{
    AuthLoginRateLimitResult result;
    if (!LoginRateLimitEnabled())
    {
        return result;
    }

    const int window_sec = LoginRateLimitWindowSec();
    const int email_max = LoginRateLimitEmailMaxFailures();
    const int ip_max = LoginRateLimitIpMaxFailures();
    for (const auto& key : FailureKeysFor(email, request, true))
    {
        const bool is_ip_key = key.rfind(kIpPrefix, 0) == 0;
        ApplyCounterLimit(result, ReadCounter(key, window_sec), is_ip_key ? ip_max : email_max, window_sec, false, key);
    }
    return result;
}

AuthLoginRateLimitResult RecordLoginFailure(const std::string& email,
                                            const memochat::gate::routing::GateRequest& request)
{
    AuthLoginRateLimitResult result;
    if (!LoginRateLimitEnabled())
    {
        return result;
    }

    const int window_sec = LoginRateLimitWindowSec();
    const int email_max = LoginRateLimitEmailMaxFailures();
    const int ip_max = LoginRateLimitIpMaxFailures();
    for (const auto& key : FailureKeysFor(email, request, true))
    {
        const bool is_ip_key = key.rfind(kIpPrefix, 0) == 0;
        ApplyCounterLimit(result,
                          IncrementCounter(key, window_sec),
                          is_ip_key ? ip_max : email_max,
                          window_sec,
                          true,
                          key);
    }
    return result;
}

void ClearLoginFailureCounters(const std::string& email, const memochat::gate::routing::GateRequest& request)
{
    (void) request;
    if (!LoginRateLimitEnabled())
    {
        return;
    }

    // Do not clear the shared IP bucket on success; otherwise one valid account
    // could reset brute-force pressure for every email behind the same IP.
    DeleteKeys(FailureKeysFor(email, request, false));
}

} // namespace gateauthsupport

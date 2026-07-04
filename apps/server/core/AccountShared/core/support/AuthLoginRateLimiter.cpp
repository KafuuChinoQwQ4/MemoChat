#include "AuthLoginRateLimiter.hpp"

#include "common_lua_scripts.hpp"
#include "ConfigMgr.hpp"
#include "RedisMgr.hpp"
#include "logging/Logger.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace gateauthsupport
{
namespace
{

constexpr const char* kSection = "AuthLoginRateLimit";
constexpr const char* kEmailPrefix = "auth_login_fail_email:";
constexpr const char* kIpPrefix = "auth_login_fail_ip:";
constexpr int kDefaultWindowSec = 900;
constexpr int kDefaultEmailMaxFailures = 8;
constexpr int kDefaultIpMaxFailures = 40;

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

int ConfigInt(const std::string& key, int default_value, int min_value, int max_value)
{
    const auto raw = ConfigMgr::Inst().GetValue(kSection, key);
    if (raw.empty())
    {
        return default_value;
    }
    try
    {
        return std::clamp(std::stoi(raw), min_value, max_value);
    }
    catch (...)
    {
        return default_value;
    }
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
    long long count = 0;
    int ttl_sec = 0;
};

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
    try
    {
        result.count = std::stoll(reply->str);
    }
    catch (...)
    {
        result.count = 0;
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
                       bool after_increment)
{
    if (!counter.ok)
    {
        result.redis_error = true;
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
    return ConfigInt("WindowSec", kDefaultWindowSec, 60, 86400);
}

int LoginRateLimitEmailMaxFailures()
{
    return ConfigInt("EmailMaxFailures", kDefaultEmailMaxFailures, 1, 1000);
}

int LoginRateLimitIpMaxFailures()
{
    return ConfigInt("IpMaxFailures", kDefaultIpMaxFailures, 1, 10000);
}

std::string ResolveLoginRateLimitClientIp(const memochat::gate::routing::GateRequest& request)
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
    return Trim(request.remote_address);
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
        ApplyCounterLimit(result, ReadCounter(key, window_sec), is_ip_key ? ip_max : email_max, window_sec, false);
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
        ApplyCounterLimit(result, IncrementCounter(key, window_sec), is_ip_key ? ip_max : email_max, window_sec, true);
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

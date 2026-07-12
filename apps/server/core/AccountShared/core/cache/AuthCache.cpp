#include "AuthCache.hpp"

#include "RedisMgr.hpp"
#include "random/Uuid.hpp"
#include "support/UserTokenValidator.hpp"

import memochat.account.auth_cache_algorithms;

namespace auth_cache_modules = memochat::account::auth_cache::modules;

namespace
{

std::string BuildVerificationCodeKey(const std::string& email)
{
    return std::string(auth_cache_modules::VerificationCodePrefix()) + email;
}

std::string BuildHttpTokenKey(int uid)
{
    return std::string(auth_cache_modules::HttpTokenPrefix()) + std::to_string(uid);
}

std::string BuildLoginProfileKey(const std::string& email)
{
    return std::string(auth_cache_modules::LoginProfilePrefix()) + email;
}

std::string BuildLoginProfileUidKey(int uid)
{
    return std::string(auth_cache_modules::LoginProfileUidPrefix()) + std::to_string(uid);
}

std::string BuildCredentialMutationLockKey(int uid)
{
    return "auth_credential_mutation_lock_" + std::to_string(uid);
}

} // namespace

namespace memochat::gate::core
{

AuthCache& AuthCache::Instance()
{
    static AuthCache instance;
    return instance;
}

bool AuthCache::GetVerificationCode(const std::string& email, std::string& code)
{
    return RedisMgr::GetInstance()->Get(BuildVerificationCodeKey(email), code);
}

bool AuthCache::DeleteVerificationCode(const std::string& email)
{
    return RedisMgr::GetInstance()->Del(BuildVerificationCodeKey(email));
}

bool AuthCache::ConsumeVerificationCode(const std::string& email, const std::string& expected_code)
{
    if (email.empty() || expected_code.empty())
    {
        return false;
    }
    static const std::string script =
        "if redis.call('GET', KEYS[1]) == ARGV[1] then return redis.call('DEL', KEYS[1]) else return 0 end";
    return RedisMgr::GetInstance()->Eval(script, {BuildVerificationCodeKey(email)}, {expected_code}) == 1;
}

bool AuthCache::GetHttpToken(int uid, std::string& token)
{
    return RedisMgr::GetInstance()->Get(BuildHttpTokenKey(uid), token);
}

bool AuthCache::SetHttpToken(int uid, const std::string& token, int ttl_seconds)
{
    return memochat::auth::StoreUserToken(uid, token, ttl_seconds);
}

bool AuthCache::DeleteHttpToken(int uid)
{
    return memochat::auth::DeleteUserToken(uid);
}

bool AuthCache::LoadLoginProfileJson(const std::string& email, std::string& payload)
{
    return RedisMgr::GetInstance()->Get(BuildLoginProfileKey(email), payload);
}

void AuthCache::StoreLoginProfileJson(const std::string& email, const std::string& payload, int ttl_seconds)
{
    RedisMgr::GetInstance()->SetEx(BuildLoginProfileKey(email), payload, ttl_seconds);
}

void AuthCache::StoreLoginProfileUid(int uid, const std::string& email, int ttl_seconds)
{
    RedisMgr::GetInstance()->SetEx(BuildLoginProfileUidKey(uid), email, ttl_seconds);
}

bool AuthCache::DeleteLoginProfileByEmail(const std::string& email)
{
    return RedisMgr::GetInstance()->Del(BuildLoginProfileKey(email));
}

bool AuthCache::LoadLoginProfileEmailByUid(int uid, std::string& email)
{
    return RedisMgr::GetInstance()->Get(BuildLoginProfileUidKey(uid), email);
}

bool AuthCache::DeleteLoginProfileUid(int uid)
{
    return RedisMgr::GetInstance()->Del(BuildLoginProfileUidKey(uid));
}

bool AuthCache::TryAcquireCredentialMutationLock(int uid, std::string& identifier)
{
    identifier.clear();
    if (uid <= 0 || !memochat::random::GenerateUuid(identifier, nullptr))
    {
        return false;
    }
    static const std::string script =
        "if redis.call('SET', KEYS[1], ARGV[1], 'NX', 'PX', ARGV[2]) then return 1 else return 0 end";
    const auto result =
        RedisMgr::GetInstance()->Eval(script, {BuildCredentialMutationLockKey(uid)}, {identifier, "60000"});
    if (result != 1)
    {
        identifier.clear();
        return false;
    }
    return true;
}

bool AuthCache::ReleaseCredentialMutationLock(int uid, const std::string& identifier)
{
    if (uid <= 0 || identifier.empty())
    {
        return false;
    }
    static const std::string script =
        "if redis.call('GET', KEYS[1]) == ARGV[1] then return redis.call('DEL', KEYS[1]) else return 0 end";
    return RedisMgr::GetInstance()->Eval(script, {BuildCredentialMutationLockKey(uid)}, {identifier}) == 1;
}

} // namespace memochat::gate::core

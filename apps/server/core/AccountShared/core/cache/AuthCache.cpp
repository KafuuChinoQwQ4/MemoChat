#include "AuthCache.hpp"

#include "RedisMgr.hpp"
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

void AuthCache::DeleteVerificationCode(const std::string& email)
{
    RedisMgr::GetInstance()->Del(BuildVerificationCodeKey(email));
}

bool AuthCache::GetHttpToken(int uid, std::string& token)
{
    return RedisMgr::GetInstance()->Get(BuildHttpTokenKey(uid), token);
}

bool AuthCache::SetHttpToken(int uid, const std::string& token, int ttl_seconds)
{
    return memochat::auth::StoreUserToken(uid, token, ttl_seconds);
}

void AuthCache::DeleteHttpToken(int uid)
{
    memochat::auth::DeleteUserToken(uid);
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

void AuthCache::DeleteLoginProfileByEmail(const std::string& email)
{
    RedisMgr::GetInstance()->Del(BuildLoginProfileKey(email));
}

bool AuthCache::LoadLoginProfileEmailByUid(int uid, std::string& email)
{
    return RedisMgr::GetInstance()->Get(BuildLoginProfileUidKey(uid), email);
}

void AuthCache::DeleteLoginProfileUid(int uid)
{
    RedisMgr::GetInstance()->Del(BuildLoginProfileUidKey(uid));
}

} // namespace memochat::gate::core

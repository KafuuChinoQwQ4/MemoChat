#include "AuthCache.h"

#include "RedisMgr.h"
#include "const.h"

namespace
{

std::string BuildVerificationCodeKey(const std::string& email)
{
    return CODEPREFIX + email;
}

std::string BuildHttpTokenKey(int uid)
{
    return USERTOKENPREFIX + std::to_string(uid);
}

std::string BuildLoginProfileKey(const std::string& email)
{
    return "ulogin_profile_" + email;
}

std::string BuildLoginProfileUidKey(int uid)
{
    return "ulogin_profile_uid_" + std::to_string(uid);
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

bool AuthCache::GetHttpToken(int uid, std::string& token)
{
    return RedisMgr::GetInstance()->Get(BuildHttpTokenKey(uid), token);
}

void AuthCache::SetHttpToken(int uid, const std::string& token)
{
    RedisMgr::GetInstance()->Set(BuildHttpTokenKey(uid), token);
}

void AuthCache::DeleteHttpToken(int uid)
{
    RedisMgr::GetInstance()->Del(BuildHttpTokenKey(uid));
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

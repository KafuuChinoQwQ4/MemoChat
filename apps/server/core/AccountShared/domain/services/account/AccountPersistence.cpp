#include "services/account/AccountPersistence.hpp"

#include "PostgresMgr.hpp"
#include "auth/RefreshToken.hpp"

namespace memochat::gate::services::account
{

namespace
{

AccountProfile ToAccountProfile(const ::UserInfo& userInfo)
{
    AccountProfile profile;
    profile.name = userInfo.name;
    profile.uid = userInfo.uid;
    profile.user_id = userInfo.user_id;
    profile.email = userInfo.email;
    profile.nick = userInfo.nick;
    profile.icon = userInfo.icon;
    profile.desc = userInfo.desc;
    profile.sex = userInfo.sex;
    return profile;
}

} // namespace

AccountPersistence& AccountPersistence::Instance()
{
    static AccountPersistence instance;
    return instance;
}

int AccountPersistence::RegisterUser(const std::string& name,
                                     const std::string& email,
                                     const std::string& password,
                                     const std::string& icon)
{
    return PostgresMgr::GetInstance()->RegUser(name, email, password, icon);
}

std::string AccountPersistence::GetUserPublicId(int uid)
{
    return PostgresMgr::GetInstance()->GetUserPublicId(uid);
}

bool AccountPersistence::FindUserByEmail(const std::string& email, int& uid, std::string& name)
{
    uid = 0;
    name.clear();
    return PostgresMgr::GetInstance()->TestProcedure(email, uid, name);
}

bool AccountPersistence::EmailMatchesUser(const std::string& name, const std::string& email)
{
    return PostgresMgr::GetInstance()->CheckEmail(name, email);
}

bool AccountPersistence::UpdatePassword(const std::string& email, const std::string& password)
{
    return PostgresMgr::GetInstance()->UpdatePwd(email, password);
}

bool AccountPersistence::CheckPassword(const std::string& email, const std::string& password, AccountProfile& profile)
{
    ::UserInfo userInfo;
    if (!PostgresMgr::GetInstance()->CheckPwd(email, password, userInfo))
    {
        return false;
    }

    profile = ToAccountProfile(userInfo);
    return true;
}

bool AccountPersistence::GetUserProfile(int uid, AccountProfile& profile)
{
    ::UserInfo userInfo;
    if (!PostgresMgr::GetInstance()->GetUserInfo(uid, userInfo))
    {
        return false;
    }

    profile = ToAccountProfile(userInfo);
    return true;
}

RefreshTokenIssueResult AccountPersistence::IssueRefreshToken(int uid,
                                                              int ttl_seconds,
                                                              const std::string& user_agent,
                                                              const std::string& ip_hash)
{
    RefreshTokenIssueResult result;
    memochat::auth::GeneratedRefreshToken generated;
    if (!memochat::auth::GenerateRefreshToken(generated))
    {
        return result;
    }

    if (!PostgresMgr::GetInstance()
             ->IssueRefreshToken(uid, generated.selector, generated.verifier_hash, ttl_seconds, user_agent, ip_hash))
    {
        return result;
    }

    result.ok = true;
    result.refresh_token = generated.token;
    return result;
}

RefreshTokenRotateResult AccountPersistence::RotateRefreshToken(const std::string& refresh_token,
                                                                int ttl_seconds,
                                                                const std::string& user_agent,
                                                                const std::string& ip_hash)
{
    RefreshTokenRotateResult result;
    memochat::auth::RefreshTokenParts parts;
    if (!memochat::auth::SplitRefreshToken(refresh_token, parts))
    {
        result.status = RefreshTokenStatus::Invalid;
        return result;
    }

    memochat::auth::GeneratedRefreshToken replacement;
    if (!memochat::auth::GenerateRefreshToken(replacement))
    {
        result.status = RefreshTokenStatus::StorageError;
        return result;
    }

    int uid = 0;
    const auto status = PostgresMgr::GetInstance()->RotateRefreshToken(parts.selector,
                                                                       parts.verifier,
                                                                       replacement.selector,
                                                                       replacement.verifier_hash,
                                                                       ttl_seconds,
                                                                       user_agent,
                                                                       ip_hash,
                                                                       uid);
    result.uid = uid;
    switch (status)
    {
        case RefreshTokenRotationStatus::Success:
            result.status = RefreshTokenStatus::Success;
            break;
        case RefreshTokenRotationStatus::Expired:
            result.status = RefreshTokenStatus::Expired;
            break;
        case RefreshTokenRotationStatus::Reused:
            result.status = RefreshTokenStatus::Reused;
            break;
        case RefreshTokenRotationStatus::StorageError:
            result.status = RefreshTokenStatus::StorageError;
            break;
        case RefreshTokenRotationStatus::Invalid:
        default:
            result.status = RefreshTokenStatus::Invalid;
            break;
    }

    if (result.status != RefreshTokenStatus::Success)
    {
        return result;
    }

    if (!GetUserProfile(uid, result.profile))
    {
        result.status = RefreshTokenStatus::StorageError;
        result.refresh_token.clear();
        return result;
    }

    result.refresh_token = replacement.token;
    return result;
}

bool AccountPersistence::RevokeRefreshToken(const std::string& refresh_token, int& uid)
{
    uid = 0;
    memochat::auth::RefreshTokenParts parts;
    if (!memochat::auth::SplitRefreshToken(refresh_token, parts))
    {
        return false;
    }
    return PostgresMgr::GetInstance()->RevokeRefreshToken(parts.selector, parts.verifier, uid);
}

bool AccountPersistence::RevokeAllRefreshTokensForUid(int uid)
{
    return PostgresMgr::GetInstance()->RevokeAllRefreshTokensForUid(uid);
}

} // namespace memochat::gate::services::account

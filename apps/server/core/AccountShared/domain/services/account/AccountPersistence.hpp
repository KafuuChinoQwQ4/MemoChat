#pragma once

#include <string>

namespace memochat::gate::services::account
{

struct AccountProfile
{
    std::string name;
    int uid = 0;
    std::string user_id;
    std::string email;
    std::string nick;
    std::string icon;
    std::string desc;
    int sex = 0;
};

enum class RefreshTokenStatus
{
    Success,
    Invalid,
    Expired,
    Reused,
    StorageError,
};

struct RefreshTokenIssueResult
{
    bool ok = false;
    std::string refresh_token;
};

struct RefreshTokenRotateResult
{
    RefreshTokenStatus status = RefreshTokenStatus::Invalid;
    int uid = 0;
    std::string refresh_token;
    AccountProfile profile;
};

class AccountPersistence
{
public:
    static AccountPersistence& Instance();

    int RegisterUser(const std::string& name,
                     const std::string& email,
                     const std::string& password,
                     const std::string& icon);
    std::string GetUserPublicId(int uid);
    bool FindUserByEmail(const std::string& email, int& uid, std::string& name);
    bool EmailMatchesUser(const std::string& name, const std::string& email);
    bool UpdatePassword(const std::string& email, const std::string& password);
    bool CheckPassword(const std::string& email, const std::string& password, AccountProfile& profile);
    bool GetUserProfile(int uid, AccountProfile& profile);
    bool UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon);
    RefreshTokenIssueResult
    IssueRefreshToken(int uid, int ttl_seconds, const std::string& user_agent, const std::string& ip_hash);
    RefreshTokenRotateResult RotateRefreshToken(const std::string& refresh_token,
                                                int ttl_seconds,
                                                const std::string& user_agent,
                                                const std::string& ip_hash);
    bool RevokeRefreshToken(const std::string& refresh_token, int& uid);
    bool RevokeAllRefreshTokensForUid(int uid);

private:
    AccountPersistence() = default;
};

} // namespace memochat::gate::services::account

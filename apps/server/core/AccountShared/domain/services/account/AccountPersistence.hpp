#pragma once

#include <cstdint>
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

enum class R18AccessState : int
{
    Denied = 0,
    Allowed = 1,
    Revoked = 2,
};

struct R18AccessPolicy
{
    int64_t adult_attested_at_ms = 0;
    R18AccessState r18_access_state = R18AccessState::Denied;

    [[nodiscard]] bool Allowed() const noexcept
    {
        return adult_attested_at_ms > 0 && r18_access_state == R18AccessState::Allowed;
    }
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
    bool GetR18AccessPolicy(int uid, R18AccessPolicy& policy);
    bool AttestAdultForR18(int uid, int64_t attested_at_ms, R18AccessPolicy& policy);
    RefreshTokenIssueResult
    IssueRefreshToken(int uid, int ttl_seconds, const std::string& user_agent, const std::string& ip_hash);
    RefreshTokenRotateResult RotateRefreshToken(const std::string& refresh_token,
                                                int ttl_seconds,
                                                const std::string& user_agent,
                                                const std::string& ip_hash);
    bool ResolveActiveRefreshTokenUserId(const std::string& refresh_token, int& uid);
    bool IsRefreshTokenActiveForUid(const std::string& refresh_token, int uid);
    bool RevokeRefreshToken(const std::string& refresh_token, int& uid);
    bool RevokeAllRefreshTokensForUid(int uid);

private:
    AccountPersistence() = default;
};

} // namespace memochat::gate::services::account

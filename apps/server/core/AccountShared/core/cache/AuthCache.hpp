#pragma once

#include <string>

namespace memochat::gate::core
{

class AuthCache
{
public:
    static AuthCache& Instance();

    bool GetVerificationCode(const std::string& email, std::string& code);
    bool DeleteVerificationCode(const std::string& email);
    bool ConsumeVerificationCode(const std::string& email, const std::string& expected_code);
    bool GetHttpToken(int uid, std::string& token);
    bool SetHttpToken(int uid, const std::string& token, int ttl_seconds);
    bool DeleteHttpToken(int uid);
    bool LoadLoginProfileJson(const std::string& email, std::string& payload);
    void StoreLoginProfileJson(const std::string& email, const std::string& payload, int ttl_seconds);
    void StoreLoginProfileUid(int uid, const std::string& email, int ttl_seconds);
    bool DeleteLoginProfileByEmail(const std::string& email);
    bool LoadLoginProfileEmailByUid(int uid, std::string& email);
    bool DeleteLoginProfileUid(int uid);
    bool TryAcquireCredentialMutationLock(int uid, std::string& identifier);
    bool ReleaseCredentialMutationLock(int uid, const std::string& identifier);

private:
    AuthCache() = default;
};

} // namespace memochat::gate::core

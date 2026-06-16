#pragma once

#include <string>

namespace memochat::gate::core
{

class AuthCache
{
public:
    static AuthCache& Instance();

    bool GetVerificationCode(const std::string& email, std::string& code);
    bool GetHttpToken(int uid, std::string& token);
    void SetHttpToken(int uid, const std::string& token);
    void DeleteHttpToken(int uid);
    bool LoadLoginProfileJson(const std::string& email, std::string& payload);
    void StoreLoginProfileJson(const std::string& email, const std::string& payload, int ttl_seconds);
    void StoreLoginProfileUid(int uid, const std::string& email, int ttl_seconds);
    void DeleteLoginProfileByEmail(const std::string& email);
    bool LoadLoginProfileEmailByUid(int uid, std::string& email);
    void DeleteLoginProfileUid(int uid);

private:
    AuthCache() = default;
};

} // namespace memochat::gate::core

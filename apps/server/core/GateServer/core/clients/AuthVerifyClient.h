#pragma once

#include <string>

namespace memochat::gate::core
{

struct VerifyCodeRequestResult
{
    int error;
    std::string email;
};

class AuthVerifyClient
{
public:
    static AuthVerifyClient& Instance();

    VerifyCodeRequestResult RequestVerifyCode(const std::string& email);

private:
    AuthVerifyClient() = default;
};

} // namespace memochat::gate::core

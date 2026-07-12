#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <string_view>
#include <utility>

namespace memochat::auth
{
inline constexpr std::string_view kWellKnownDevHmacSecret = "memochat-dev-chat-secret";
inline constexpr std::string_view kWellKnownDevJwtAccessSecret = "memochat-dev-access-token-secret";
inline bool IsWellKnownDevHmacSecret(std::string_view secret)
{
    return secret == kWellKnownDevHmacSecret;
}

inline bool IsWellKnownDevJwtAccessSecret(std::string_view secret)
{
    return secret == kWellKnownDevJwtAccessSecret;
}

inline std::string LowerAscii(std::string value)
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

inline bool IsTruthyEnvValue(std::string value)
{
    value = LowerAscii(std::move(value));
    return value == "1" || value == "true" || value == "yes" || value == "on";
}

inline bool IsDevSecretsAllowed()
{
    if (const char* allowed = std::getenv("MEMOCHAT_ALLOW_DEV_SECRETS"); allowed != nullptr)
    {
        return IsTruthyEnvValue(allowed);
    }
    return false;
}

inline bool IsProductionSecretEnforcementEnabled()
{
    return !IsDevSecretsAllowed();
}

inline bool
RequireNonDefaultChatAuthSecretInProduction(std::string_view service_name, std::string_view secret, std::string& error)
{
    error.clear();
    if (!IsProductionSecretEnforcementEnabled())
    {
        return true;
    }
    if (secret.empty() || IsWellKnownDevHmacSecret(secret))
    {
        error = std::string(service_name) + " refuses to start: ChatAuth.HmacSecret must be non-default; set "
                                            "MEMOCHAT_ALLOW_DEV_SECRETS=1 only for local development";
        return false;
    }
    if (secret.size() < 32)
    {
        error = std::string(service_name) + " refuses to start: ChatAuth.HmacSecret must be at least 32 bytes";
        return false;
    }
    return true;
}

inline bool
RequireNonDefaultJwtAccessSecretInProduction(std::string_view service_name, std::string_view secret, std::string& error)
{
    error.clear();
    if (!IsProductionSecretEnforcementEnabled())
    {
        return true;
    }
    if (secret.empty() || IsWellKnownDevJwtAccessSecret(secret))
    {
        error = std::string(service_name) + " refuses to start: AuthToken.JwtSecret must be non-default; set "
                                            "MEMOCHAT_ALLOW_DEV_SECRETS=1 only for local development";
        return false;
    }
    if (secret.size() < 32)
    {
        error = std::string(service_name) + " refuses to start: AuthToken.JwtSecret must be at least 32 bytes";
        return false;
    }
    return true;
}
} // namespace memochat::auth

#pragma once
#include <string_view>
namespace memochat::auth
{
inline constexpr std::string_view kWellKnownDevHmacSecret = "memochat-dev-chat-secret";
inline bool IsWellKnownDevHmacSecret(std::string_view secret)
{
    return secret == kWellKnownDevHmacSecret;
}
} // namespace memochat::auth

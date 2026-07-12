#pragma once
#include <string>
namespace memochat::auth
{
// Validates a JWT access token against the shared HTTP-auth contract:
// signature/expiry/issuer/audience must be valid, claims.uid must match `uid`,
// and Redis must still bind "utoken_<uid>" to this exact access token.
bool ValidateUserToken(int uid, const std::string& token);
bool ResolveUserIdFromToken(const std::string& token, int& uid);
bool StoreUserToken(int uid, const std::string& token, int ttl_seconds);
bool DeleteUserToken(int uid);
} // namespace memochat::auth

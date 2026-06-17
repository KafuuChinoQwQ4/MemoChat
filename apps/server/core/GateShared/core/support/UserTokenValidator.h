#pragma once
#include <string>
namespace memochat::auth
{
// Validates a user session token against the Redis contract used across all
// focused services and ChatServer: GET "utoken_<uid>" must equal `token`.
// Returns false for uid <= 0, empty token, Redis miss, or mismatch.
bool ValidateUserToken(int uid, const std::string& token);
} // namespace memochat::auth

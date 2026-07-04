#pragma once

#include <string>

namespace memochat::auth
{

bool HashPassword(const std::string& password, std::string& password_hash);
bool VerifyPassword(const std::string& password_hash, const std::string& password);
bool NeedsPasswordRehash(const std::string& password_hash);
bool LooksLikePasswordHash(const std::string& password_hash);

} // namespace memochat::auth

#pragma once

#include <string>
#include <string_view>

namespace memochat::auth
{

struct RefreshTokenParts
{
    std::string selector;
    std::string verifier;
};

struct GeneratedRefreshToken
{
    std::string token;
    std::string selector;
    std::string verifier_hash;
};

bool GenerateRefreshToken(GeneratedRefreshToken& out);
bool SplitRefreshToken(std::string_view token, RefreshTokenParts& out);
bool HashRefreshTokenVerifier(std::string_view verifier, std::string& verifier_hash);
bool VerifyRefreshTokenVerifier(std::string_view verifier, std::string_view verifier_hash);
bool HashRefreshTokenMetadata(std::string_view material, std::string& material_hash);
bool LooksLikeRefreshTokenHash(std::string_view hash);

} // namespace memochat::auth

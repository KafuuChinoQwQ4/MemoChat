#include "auth/RefreshToken.hpp"

#include <sodium.h>

#include <array>
#include <cstddef>

namespace memochat::auth
{
namespace
{
constexpr std::string_view kHashPrefix = "sodium-generichash-v1$";
constexpr std::size_t kSelectorBytes = 18;
constexpr std::size_t kVerifierBytes = 32;
constexpr std::size_t kDigestBytes = 32;

bool EnsureSodiumInitialized()
{
    static const bool initialized = sodium_init() >= 0;
    return initialized;
}

std::string HexEncode(const unsigned char* data, std::size_t size)
{
    static constexpr char kHex[] = "0123456789abcdef";
    std::string out;
    out.reserve(size * 2);
    for (std::size_t i = 0; i < size; ++i)
    {
        const unsigned char byte = data[i];
        out.push_back(kHex[(byte >> 4) & 0x0F]);
        out.push_back(kHex[byte & 0x0F]);
    }
    return out;
}

bool IsLowerHex(std::string_view value)
{
    for (const char ch : value)
    {
        if (!((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f')))
        {
            return false;
        }
    }
    return true;
}

bool HashMaterial(std::string_view material, std::string& out_hash)
{
    out_hash.clear();
    if (material.empty() || !EnsureSodiumInitialized())
    {
        return false;
    }

    std::array<unsigned char, kDigestBytes> digest{};
    if (crypto_generichash(digest.data(),
                           digest.size(),
                           reinterpret_cast<const unsigned char*>(material.data()),
                           material.size(),
                           nullptr,
                           0) != 0)
    {
        return false;
    }

    out_hash = std::string(kHashPrefix) + HexEncode(digest.data(), digest.size());
    return true;
}
} // namespace

bool GenerateRefreshToken(GeneratedRefreshToken& out)
{
    out = {};
    if (!EnsureSodiumInitialized())
    {
        return false;
    }

    std::array<unsigned char, kSelectorBytes> selector_bytes{};
    std::array<unsigned char, kVerifierBytes> verifier_bytes{};
    randombytes_buf(selector_bytes.data(), selector_bytes.size());
    randombytes_buf(verifier_bytes.data(), verifier_bytes.size());

    out.selector = HexEncode(selector_bytes.data(), selector_bytes.size());
    const std::string verifier = HexEncode(verifier_bytes.data(), verifier_bytes.size());
    if (!HashRefreshTokenVerifier(verifier, out.verifier_hash))
    {
        out = {};
        return false;
    }

    out.token = out.selector + "." + verifier;
    return true;
}

bool SplitRefreshToken(std::string_view token, RefreshTokenParts& out)
{
    out = {};
    const auto separator = token.find('.');
    if (separator == std::string_view::npos || token.find('.', separator + 1) != std::string_view::npos)
    {
        return false;
    }

    const auto selector = token.substr(0, separator);
    const auto verifier = token.substr(separator + 1);
    if (selector.size() != kSelectorBytes * 2 || verifier.size() != kVerifierBytes * 2)
    {
        return false;
    }
    if (!IsLowerHex(selector) || !IsLowerHex(verifier))
    {
        return false;
    }

    out.selector = std::string(selector);
    out.verifier = std::string(verifier);
    return true;
}

bool HashRefreshTokenVerifier(std::string_view verifier, std::string& verifier_hash)
{
    return HashMaterial(verifier, verifier_hash);
}

bool VerifyRefreshTokenVerifier(std::string_view verifier, std::string_view verifier_hash)
{
    std::string computed_hash;
    if (!HashRefreshTokenVerifier(verifier, computed_hash))
    {
        return false;
    }
    if (computed_hash.size() != verifier_hash.size())
    {
        return false;
    }
    return sodium_memcmp(computed_hash.data(), verifier_hash.data(), computed_hash.size()) == 0;
}

bool HashRefreshTokenMetadata(std::string_view material, std::string& material_hash)
{
    if (material.empty())
    {
        material_hash.clear();
        return true;
    }
    return HashMaterial(material, material_hash);
}

bool LooksLikeRefreshTokenHash(std::string_view hash)
{
    if (!hash.starts_with(kHashPrefix))
    {
        return false;
    }
    const auto hex = hash.substr(kHashPrefix.size());
    return hex.size() == kDigestBytes * 2 && IsLowerHex(hex);
}

} // namespace memochat::auth

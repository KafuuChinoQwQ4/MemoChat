#include "support/UserTokenValidator.hpp"

#include "RedisMgr.hpp"
#include "const.hpp"

#include <array>
#include <charconv>
#include <cstddef>
#include <string>
#include <string_view>

#include <sodium.h>

import memochat.gate.user_token_validator_algorithms;

namespace memochat::auth
{
namespace
{
constexpr std::string_view kUserTokenLookupPrefix = "utoken_lookup_";
constexpr std::size_t kTokenLookupDigestBytes = 32;

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
        out.push_back(kHex[(byte >> 4U) & 0x0FU]);
        out.push_back(kHex[byte & 0x0FU]);
    }
    return out;
}

std::string UserTokenLookupKey(std::string_view token)
{
    if (token.empty() || !EnsureSodiumInitialized())
    {
        return "";
    }
    std::array<unsigned char, kTokenLookupDigestBytes> digest{};
    if (crypto_generichash(digest.data(),
                           digest.size(),
                           reinterpret_cast<const unsigned char*>(token.data()),
                           token.size(),
                           nullptr,
                           0) != 0)
    {
        return "";
    }
    return std::string(kUserTokenLookupPrefix) + HexEncode(digest.data(), digest.size());
}

bool ParsePositiveInt(std::string_view value, int& parsed)
{
    int out = 0;
    const auto* begin = value.data();
    const auto* end = value.data() + value.size();
    const auto [ptr, ec] = std::from_chars(begin, end, out);
    if (ec != std::errc{} || ptr != end || out <= 0)
    {
        return false;
    }
    parsed = out;
    return true;
}
} // namespace

bool ValidateUserToken(int uid, const std::string& token)
{
    if (!memochat::gate::auth::modules::ShouldValidateUserToken(uid, token.empty()))
    {
        return false;
    }
    const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    std::string token_value;
    const bool redis_hit = RedisMgr::GetInstance()->Get(token_key, token_value);
    return memochat::gate::auth::modules::ShouldAcceptStoredUserToken(redis_hit,
                                                                      token_value.empty(),
                                                                      token_value == token);
}

bool ResolveUserIdFromToken(const std::string& token, int& uid)
{
    uid = 0;
    const auto lookup_key = UserTokenLookupKey(token);
    if (lookup_key.empty())
    {
        return false;
    }
    std::string raw_uid;
    if (!RedisMgr::GetInstance()->Get(lookup_key, raw_uid))
    {
        return false;
    }
    int resolved_uid = 0;
    if (!ParsePositiveInt(raw_uid, resolved_uid) || !ValidateUserToken(resolved_uid, token))
    {
        return false;
    }
    uid = resolved_uid;
    return true;
}

bool StoreUserToken(int uid, const std::string& token, int ttl_seconds)
{
    if (!memochat::gate::auth::modules::ShouldValidateUserToken(uid, token.empty()) || ttl_seconds <= 0)
    {
        return false;
    }
    const auto lookup_key = UserTokenLookupKey(token);
    if (lookup_key.empty())
    {
        return false;
    }
    const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    if (!RedisMgr::GetInstance()->SetEx(token_key, token, ttl_seconds))
    {
        return false;
    }
    if (!RedisMgr::GetInstance()->SetEx(lookup_key, std::to_string(uid), ttl_seconds))
    {
        RedisMgr::GetInstance()->Del(token_key);
        return false;
    }
    return true;
}

void DeleteUserToken(int uid)
{
    if (uid <= 0)
    {
        return;
    }
    const std::string token_key = USERTOKENPREFIX + std::to_string(uid);
    std::string token;
    if (RedisMgr::GetInstance()->Get(token_key, token))
    {
        const auto lookup_key = UserTokenLookupKey(token);
        if (!lookup_key.empty())
        {
            RedisMgr::GetInstance()->Del(lookup_key);
        }
    }
    RedisMgr::GetInstance()->Del(token_key);
}
} // namespace memochat::auth

#include "auth/JwtAccessToken.hpp"

#include "json/GlazeCompat.hpp"

#include <array>
#include <charconv>
#include <cstdint>
#include <ctime>
#include <string>
#include <string_view>
#include <utility>

#include <sodium.h>

namespace
{

struct JwtHeader
{
    std::string alg;
    std::string typ;
};

} // namespace

template <> struct glz::meta<JwtHeader>
{
    using T = JwtHeader;
    static constexpr auto value = glz::object("alg", &T::alg, "typ", &T::typ);
};

namespace memochat::auth
{
namespace
{

bool EnsureSodiumInitialized()
{
    static const bool initialized = sodium_init() >= 0;
    return initialized;
}

std::string Base64UrlEncode(std::string_view input)
{
    static constexpr char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    std::string out;
    out.reserve(((input.size() + 2) / 3) * 4);
    uint32_t val = 0;
    int valb = -6;
    for (unsigned char c : input)
    {
        val = (val << 8U) | c;
        valb += 8;
        while (valb >= 0)
        {
            out.push_back(table[(val >> valb) & 0x3FU]);
            valb -= 6;
        }
    }
    if (valb > -6)
    {
        out.push_back(table[((val << 8U) >> (valb + 8)) & 0x3FU]);
    }
    return out;
}

bool Base64UrlDecode(std::string_view input, std::string& output)
{
    static constexpr std::array<int, 256> table = []
    {
        std::array<int, 256> values{};
        values.fill(-1);
        for (int i = 'A'; i <= 'Z'; ++i)
        {
            values[static_cast<std::size_t>(i)] = i - 'A';
        }
        for (int i = 'a'; i <= 'z'; ++i)
        {
            values[static_cast<std::size_t>(i)] = i - 'a' + 26;
        }
        for (int i = '0'; i <= '9'; ++i)
        {
            values[static_cast<std::size_t>(i)] = i - '0' + 52;
        }
        values[static_cast<std::size_t>('-')] = 62;
        values[static_cast<std::size_t>('_')] = 63;
        return values;
    }();

    output.clear();
    output.reserve(input.size() * 3 / 4);
    uint32_t val = 0;
    int valb = -8;
    for (unsigned char c : input)
    {
        if (c == '=')
        {
            return false;
        }
        const int decoded = table[static_cast<std::size_t>(c)];
        if (decoded < 0)
        {
            return false;
        }
        val = (val << 6U) | static_cast<uint32_t>(decoded);
        valb += 6;
        if (valb >= 0)
        {
            output.push_back(static_cast<char>((val >> valb) & 0xFFU));
            valb -= 8;
        }
    }
    return true;
}

bool HmacSha256(std::string_view secret, std::string_view data, std::string& output)
{
    if (secret.empty() || !EnsureSodiumInitialized())
    {
        return false;
    }
    std::array<unsigned char, crypto_auth_hmacsha256_BYTES> digest{};
    crypto_auth_hmacsha256_state state;
    crypto_auth_hmacsha256_init(&state, reinterpret_cast<const unsigned char*>(secret.data()), secret.size());
    crypto_auth_hmacsha256_update(&state, reinterpret_cast<const unsigned char*>(data.data()), data.size());
    crypto_auth_hmacsha256_final(&state, digest.data());
    output.assign(reinterpret_cast<const char*>(digest.data()), digest.size());
    return true;
}

bool ConstantTimeEqual(std::string_view left, std::string_view right)
{
    return left.size() == right.size() && sodium_memcmp(left.data(), right.data(), left.size()) == 0;
}

int64_t EffectiveNowSec(const JwtAccessTokenValidationOptions& options)
{
    return options.now_sec > 0 ? options.now_sec : static_cast<int64_t>(time(nullptr));
}

bool ParseUidSub(std::string_view sub, int& uid)
{
    int parsed = 0;
    const auto* begin = sub.data();
    const auto* end = sub.data() + sub.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed);
    if (ec != std::errc{} || ptr != end || parsed <= 0)
    {
        return false;
    }
    uid = parsed;
    return true;
}

void SetError(std::string* error_code, std::string_view value)
{
    if (error_code != nullptr)
    {
        *error_code = value;
    }
}

} // namespace

std::string EncodeAccessToken(const JwtAccessTokenClaims& claims, const std::string& secret)
{
    JwtHeader header;
    header.alg = "HS256";
    header.typ = "JWT";

    std::string header_json;
    std::string payload_json;
    if (glz::write_json(header, header_json) || glz::write_json(claims, payload_json))
    {
        return "";
    }

    const std::string signing_input = Base64UrlEncode(header_json) + "." + Base64UrlEncode(payload_json);
    std::string mac;
    if (!HmacSha256(secret, signing_input, mac))
    {
        return "";
    }
    return signing_input + "." + Base64UrlEncode(mac);
}

bool DecodeAndVerifyAccessToken(const std::string& token,
                                const std::string& secret,
                                const JwtAccessTokenValidationOptions& options,
                                JwtAccessTokenClaims& claims,
                                std::string* error_code)
{
    if (secret.empty())
    {
        SetError(error_code, "secret");
        return false;
    }

    const auto first_sep = token.find('.');
    const auto second_sep = first_sep == std::string::npos ? std::string::npos : token.find('.', first_sep + 1);
    if (first_sep == std::string::npos || second_sep == std::string::npos ||
        token.find('.', second_sep + 1) != std::string::npos)
    {
        SetError(error_code, "format");
        return false;
    }

    const std::string signing_input = token.substr(0, second_sep);
    std::string expected_mac;
    if (!HmacSha256(secret, signing_input, expected_mac))
    {
        SetError(error_code, "signature");
        return false;
    }

    std::string header_json;
    std::string payload_json;
    std::string signature;
    if (!Base64UrlDecode(std::string_view(token).substr(0, first_sep), header_json) ||
        !Base64UrlDecode(std::string_view(token).substr(first_sep + 1, second_sep - first_sep - 1), payload_json) ||
        !Base64UrlDecode(std::string_view(token).substr(second_sep + 1), signature))
    {
        SetError(error_code, "base64");
        return false;
    }
    if (!ConstantTimeEqual(expected_mac, signature))
    {
        SetError(error_code, "signature");
        return false;
    }

    JwtHeader header;
    if (glz::read_json(header, header_json) || header.alg != "HS256" || header.typ != "JWT")
    {
        SetError(error_code, "header");
        return false;
    }

    JwtAccessTokenClaims parsed_claims;
    memochat::json::JsonValue payload_root;
    if (!memochat::json::glaze_parse(payload_root, payload_json))
    {
        SetError(error_code, "payload");
        return false;
    }
    if (options.require_token_version && !memochat::json::glaze_has_key(payload_root, "token_version"))
    {
        SetError(error_code, "token_version");
        return false;
    }
    if (glz::read_json(parsed_claims, payload_json))
    {
        SetError(error_code, "payload");
        return false;
    }

    int sub_uid = 0;
    if (parsed_claims.typ != "access" || parsed_claims.uid <= 0 || parsed_claims.jti.empty() ||
        !ParseUidSub(parsed_claims.sub, sub_uid) || sub_uid != parsed_claims.uid)
    {
        SetError(error_code, "claims");
        return false;
    }
    if (!options.issuer.empty() && parsed_claims.iss != options.issuer)
    {
        SetError(error_code, "issuer");
        return false;
    }
    if (!options.audience.empty() && parsed_claims.aud != options.audience)
    {
        SetError(error_code, "audience");
        return false;
    }
    if (options.require_token_version && parsed_claims.token_version != options.required_token_version)
    {
        SetError(error_code, "token_version");
        return false;
    }

    const int64_t now = EffectiveNowSec(options);
    const int64_t skew = options.allowed_clock_skew_sec < 0 ? 0 : options.allowed_clock_skew_sec;
    if (parsed_claims.exp <= 0 || parsed_claims.exp <= now - skew)
    {
        SetError(error_code, "expired");
        return false;
    }
    if (parsed_claims.nbf > 0 && parsed_claims.nbf > now + skew)
    {
        SetError(error_code, "not_before");
        return false;
    }
    if (parsed_claims.iat > 0 && parsed_claims.iat > now + skew)
    {
        SetError(error_code, "issued_at");
        return false;
    }

    claims = std::move(parsed_claims);
    return true;
}

bool LooksLikeJwtAccessToken(std::string_view token)
{
    const auto first_sep = token.find('.');
    const auto second_sep =
        first_sep == std::string_view::npos ? std::string_view::npos : token.find('.', first_sep + 1);
    return first_sep != std::string_view::npos && second_sep != std::string_view::npos &&
           token.find('.', second_sep + 1) == std::string_view::npos;
}

} // namespace memochat::auth

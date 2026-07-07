#include <gtest/gtest.h>

#include "auth/JwtAccessToken.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include <sodium.h>

namespace
{

constexpr int64_t kNow = 1'800'000'000;
const std::string kSecret = "0123456789abcdef0123456789abcdef";

memochat::auth::JwtAccessTokenClaims ValidClaims()
{
    memochat::auth::JwtAccessTokenClaims claims;
    claims.typ = "access";
    claims.iss = "memochat";
    claims.aud = "memochat-http";
    claims.sub = "42";
    claims.uid = 42;
    claims.iat = kNow;
    claims.nbf = kNow;
    claims.exp = kNow + 900;
    claims.jti = "access-jti-1";
    claims.token_version = 0;
    return claims;
}

memochat::auth::JwtAccessTokenValidationOptions ValidOptions()
{
    memochat::auth::JwtAccessTokenValidationOptions options;
    options.issuer = "memochat";
    options.audience = "memochat-http";
    options.now_sec = kNow + 10;
    options.allowed_clock_skew_sec = 0;
    return options;
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

std::string SignJwtParts(std::string_view header_json, std::string_view payload_json, const std::string& secret)
{
    if (sodium_init() < 0)
    {
        return "";
    }
    const std::string signing_input = Base64UrlEncode(header_json) + "." + Base64UrlEncode(payload_json);
    std::array<unsigned char, crypto_auth_hmacsha256_BYTES> digest{};
    crypto_auth_hmacsha256_state state;
    crypto_auth_hmacsha256_init(&state, reinterpret_cast<const unsigned char*>(secret.data()), secret.size());
    crypto_auth_hmacsha256_update(&state,
                                  reinterpret_cast<const unsigned char*>(signing_input.data()),
                                  signing_input.size());
    crypto_auth_hmacsha256_final(&state, digest.data());
    return signing_input + "." +
           Base64UrlEncode(std::string_view(reinterpret_cast<const char*>(digest.data()), digest.size()));
}

} // namespace

TEST(JwtAccessTokenTest, EncodesAndVerifiesAccessClaims)
{
    const auto token = memochat::auth::EncodeAccessToken(ValidClaims(), kSecret);
    ASSERT_FALSE(token.empty());
    EXPECT_TRUE(memochat::auth::LooksLikeJwtAccessToken(token));

    memochat::auth::JwtAccessTokenClaims decoded;
    std::string error;
    ASSERT_TRUE(memochat::auth::DecodeAndVerifyAccessToken(token, kSecret, ValidOptions(), decoded, &error)) << error;

    EXPECT_EQ(decoded.typ, "access");
    EXPECT_EQ(decoded.iss, "memochat");
    EXPECT_EQ(decoded.aud, "memochat-http");
    EXPECT_EQ(decoded.sub, "42");
    EXPECT_EQ(decoded.uid, 42);
    EXPECT_EQ(decoded.jti, "access-jti-1");
    EXPECT_EQ(decoded.token_version, 0);
}

TEST(JwtAccessTokenTest, RejectsTamperedSignature)
{
    auto token = memochat::auth::EncodeAccessToken(ValidClaims(), kSecret);
    ASSERT_FALSE(token.empty());
    token.back() = token.back() == 'A' ? 'B' : 'A';

    memochat::auth::JwtAccessTokenClaims decoded;
    std::string error;
    EXPECT_FALSE(memochat::auth::DecodeAndVerifyAccessToken(token, kSecret, ValidOptions(), decoded, &error));
    EXPECT_EQ(error, "signature");
}

TEST(JwtAccessTokenTest, RejectsExpiredOrFutureTokens)
{
    auto expired = ValidClaims();
    expired.exp = kNow - 1;
    auto token = memochat::auth::EncodeAccessToken(expired, kSecret);
    memochat::auth::JwtAccessTokenClaims decoded;
    std::string error;
    EXPECT_FALSE(memochat::auth::DecodeAndVerifyAccessToken(token, kSecret, ValidOptions(), decoded, &error));
    EXPECT_EQ(error, "expired");

    auto future = ValidClaims();
    future.nbf = kNow + 100;
    token = memochat::auth::EncodeAccessToken(future, kSecret);
    error.clear();
    EXPECT_FALSE(memochat::auth::DecodeAndVerifyAccessToken(token, kSecret, ValidOptions(), decoded, &error));
    EXPECT_EQ(error, "not_before");
}

TEST(JwtAccessTokenTest, RejectsIssuerAudienceOrUidMismatch)
{
    auto token = memochat::auth::EncodeAccessToken(ValidClaims(), kSecret);
    auto options = ValidOptions();
    options.audience = "other";
    memochat::auth::JwtAccessTokenClaims decoded;
    std::string error;
    EXPECT_FALSE(memochat::auth::DecodeAndVerifyAccessToken(token, kSecret, options, decoded, &error));
    EXPECT_EQ(error, "audience");

    auto bad_uid = ValidClaims();
    bad_uid.sub = "43";
    token = memochat::auth::EncodeAccessToken(bad_uid, kSecret);
    error.clear();
    EXPECT_FALSE(memochat::auth::DecodeAndVerifyAccessToken(token, kSecret, ValidOptions(), decoded, &error));
    EXPECT_EQ(error, "claims");
}

TEST(JwtAccessTokenTest, RejectsTokenVersionMismatchOrMissingClaim)
{
    auto mismatched = ValidClaims();
    mismatched.token_version = 1;
    auto token = memochat::auth::EncodeAccessToken(mismatched, kSecret);
    memochat::auth::JwtAccessTokenClaims decoded;
    std::string error;
    EXPECT_FALSE(memochat::auth::DecodeAndVerifyAccessToken(token, kSecret, ValidOptions(), decoded, &error));
    EXPECT_EQ(error, "token_version");

    token = SignJwtParts(
        R"({"alg":"HS256","typ":"JWT"})",
        R"({"typ":"access","iss":"memochat","aud":"memochat-http","sub":"42","uid":42,"iat":1800000000,"nbf":1800000000,"exp":1800000900,"jti":"access-jti-1"})",
        kSecret);
    ASSERT_FALSE(token.empty());
    error.clear();
    EXPECT_FALSE(memochat::auth::DecodeAndVerifyAccessToken(token, kSecret, ValidOptions(), decoded, &error));
    EXPECT_EQ(error, "token_version");
}

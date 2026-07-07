#pragma once

#include <glaze/glaze.hpp>

#include <cstdint>
#include <string>
#include <string_view>

namespace memochat::auth
{

struct JwtAccessTokenClaims
{
    std::string typ;
    std::string iss;
    std::string aud;
    std::string sub;
    int uid = 0;
    int64_t iat = 0;
    int64_t nbf = 0;
    int64_t exp = 0;
    std::string jti;
    int token_version = 0;
};

struct JwtAccessTokenValidationOptions
{
    std::string issuer;
    std::string audience;
    int64_t now_sec = 0;
    int64_t allowed_clock_skew_sec = 30;
    int required_token_version = 0;
    bool require_token_version = true;
};

std::string EncodeAccessToken(const JwtAccessTokenClaims& claims, const std::string& secret);

bool DecodeAndVerifyAccessToken(const std::string& token,
                                const std::string& secret,
                                const JwtAccessTokenValidationOptions& options,
                                JwtAccessTokenClaims& claims,
                                std::string* error_code = nullptr);

bool LooksLikeJwtAccessToken(std::string_view token);

} // namespace memochat::auth

template <> struct glz::meta<memochat::auth::JwtAccessTokenClaims>
{
    using T = memochat::auth::JwtAccessTokenClaims;
    static constexpr auto value = glz::object("typ",
                                              &T::typ,
                                              "iss",
                                              &T::iss,
                                              "aud",
                                              &T::aud,
                                              "sub",
                                              &T::sub,
                                              "uid",
                                              &T::uid,
                                              "iat",
                                              &T::iat,
                                              "nbf",
                                              &T::nbf,
                                              "exp",
                                              &T::exp,
                                              "jti",
                                              &T::jti,
                                              "token_version",
                                              &T::token_version);
};

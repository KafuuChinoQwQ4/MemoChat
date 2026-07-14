#include "AuthPublicDtos.hpp"

#include "json/TypedJsonCodec.hpp"

import memochat.account.auth_public_dto_algorithms;

#include <algorithm>
#include <cctype>
#include <string>

namespace
{

constexpr std::size_t kMaxEmailLength = 254;
constexpr std::size_t kMaxUserLength = 32;
constexpr std::size_t kMaxPasswordLength = 128;
constexpr std::size_t kMaxIconLength = 512;
constexpr std::size_t kMaxNickLength = 32;
constexpr std::size_t kMaxDescLength = 255;
constexpr std::size_t kMaxVerifyCodeLength = 16;
constexpr std::size_t kMaxRefreshTokenLength = 128;
constexpr std::size_t kMaxClientVersionLength = 32;
constexpr std::size_t kRefreshTokenSelectorLength = 36;
constexpr std::size_t kRefreshTokenVerifierLength = 64;
constexpr std::string_view kWebRefreshCookieName = "__Host-memochat_refresh";

std::string_view TrimAscii(std::string_view value)
{
    while (!value.empty() && (value.front() == ' ' || value.front() == '\t'))
    {
        value.remove_prefix(1);
    }
    while (!value.empty() && (value.back() == ' ' || value.back() == '\t'))
    {
        value.remove_suffix(1);
    }
    return value;
}

bool ParseJsonForAuthPublic(std::string_view body, memochat::json::JsonValue* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    if (!memochat::json::glaze_parse(*out, body))
    {
        if (error_out != nullptr)
        {
            *error_out = "invalid json";
        }
        return false;
    }
    return true;
}

template <typename Dto>
bool DecodeAuthPublicRequest(std::string_view body,
                             Dto* out,
                             memochat::json::JsonValue* parsed_root,
                             std::string* error_out,
                             Dto (*from_json)(const memochat::json::JsonValue&))
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }

    memochat::json::JsonValue root;
    if (!ParseJsonForAuthPublic(body, &root, error_out))
    {
        return false;
    }
    *out = from_json(root);
    if (parsed_root != nullptr)
    {
        *parsed_root = root;
    }
    return true;
}

template <typename T> bool WriteTypedJsonNoThrow(const T& value, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(value, out, error_out);
}

template <typename T> memochat::json::JsonValue TypedJsonToJsonValue(const T& value)
{
    std::string body;
    if (!WriteTypedJsonNoThrow(value, &body, nullptr))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }

    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    return root;
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

bool IsEmailLocalChar(char ch)
{
    const unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalnum(uch) || ch == '.' || ch == '_' || ch == '%' || ch == '+' || ch == '-';
}

bool IsDomainLabelChar(char ch)
{
    const unsigned char uch = static_cast<unsigned char>(ch);
    return std::isalnum(uch) || ch == '-';
}

bool HasNoDotEdgeOrRun(std::string_view value)
{
    return !value.empty() && value.front() != '.' && value.back() != '.' && value.find("..") == std::string_view::npos;
}

gateauthsupport::AuthInputValidationResult TooLong(gateauthsupport::AuthInputField field)
{
    return gateauthsupport::AuthInputValidationResult{.code = gateauthsupport::AuthInputValidationCode::TooLong,
                                                      .field = field,
                                                      .max_length = gateauthsupport::AuthInputMaxLength(field)};
}

gateauthsupport::AuthInputValidationResult Missing(gateauthsupport::AuthInputField field)
{
    return gateauthsupport::AuthInputValidationResult{.code = gateauthsupport::AuthInputValidationCode::MissingRequired,
                                                      .field = field,
                                                      .max_length = gateauthsupport::AuthInputMaxLength(field)};
}

gateauthsupport::AuthInputValidationResult InvalidEmail()
{
    return gateauthsupport::AuthInputValidationResult{
        .code = gateauthsupport::AuthInputValidationCode::InvalidEmail,
        .field = gateauthsupport::AuthInputField::Email,
        .max_length = gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::Email)};
}

gateauthsupport::AuthInputValidationResult InvalidRefreshToken()
{
    return gateauthsupport::AuthInputValidationResult{
        .code = gateauthsupport::AuthInputValidationCode::InvalidRefreshToken,
        .field = gateauthsupport::AuthInputField::RefreshToken,
        .max_length = gateauthsupport::AuthInputMaxLength(gateauthsupport::AuthInputField::RefreshToken)};
}

gateauthsupport::AuthInputValidationResult
ValidateStringField(std::string_view value, gateauthsupport::AuthInputField field, bool required)
{
    if (value.size() > gateauthsupport::AuthInputMaxLength(field))
    {
        return TooLong(field);
    }
    if (required && value.empty())
    {
        return Missing(field);
    }
    return {};
}

} // namespace

namespace gateauthsupport
{

const char* AuthInputFieldName(AuthInputField field)
{
    switch (field)
    {
        case AuthInputField::Email:
            return "email";
        case AuthInputField::User:
            return "user";
        case AuthInputField::Passwd:
            return "passwd";
        case AuthInputField::Confirm:
            return "confirm";
        case AuthInputField::Icon:
            return "icon";
        case AuthInputField::Nick:
            return "nick";
        case AuthInputField::Desc:
            return "desc";
        case AuthInputField::VarifyCode:
            return "varifycode";
        case AuthInputField::RefreshToken:
            return "refresh_token";
        case AuthInputField::ClientVer:
            return "client_ver";
    }
    return "unknown";
}

const char* AuthInputValidationCodeName(AuthInputValidationCode code)
{
    switch (code)
    {
        case AuthInputValidationCode::Ok:
            return "ok";
        case AuthInputValidationCode::MissingRequired:
            return "missing_required";
        case AuthInputValidationCode::TooLong:
            return "too_long";
        case AuthInputValidationCode::InvalidEmail:
            return "invalid_email";
        case AuthInputValidationCode::InvalidRefreshToken:
            return "invalid_refresh_token";
    }
    return "unknown";
}

std::size_t AuthInputMaxLength(AuthInputField field)
{
    switch (field)
    {
        case AuthInputField::Email:
            return kMaxEmailLength;
        case AuthInputField::User:
            return kMaxUserLength;
        case AuthInputField::Passwd:
        case AuthInputField::Confirm:
            return kMaxPasswordLength;
        case AuthInputField::Icon:
            return kMaxIconLength;
        case AuthInputField::Nick:
            return kMaxNickLength;
        case AuthInputField::Desc:
            return kMaxDescLength;
        case AuthInputField::VarifyCode:
            return kMaxVerifyCodeLength;
        case AuthInputField::RefreshToken:
            return kMaxRefreshTokenLength;
        case AuthInputField::ClientVer:
            return kMaxClientVersionLength;
    }
    return 0;
}

bool IsValidAuthEmail(std::string_view email)
{
    if (email.empty() || email.size() > kMaxEmailLength)
    {
        return false;
    }
    for (const char ch : email)
    {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (uch <= 32 || uch >= 127)
        {
            return false;
        }
    }

    const auto at = email.find('@');
    if (at == std::string_view::npos || email.find('@', at + 1) != std::string_view::npos || at == 0 ||
        at + 1 >= email.size())
    {
        return false;
    }

    const auto local = email.substr(0, at);
    const auto domain = email.substr(at + 1);
    if (local.size() > 64 || domain.size() > 253 || !HasNoDotEdgeOrRun(local) || !HasNoDotEdgeOrRun(domain))
    {
        return false;
    }
    if (domain.find('.') == std::string_view::npos)
    {
        return false;
    }
    if (!std::all_of(local.begin(), local.end(), IsEmailLocalChar))
    {
        return false;
    }

    std::size_t label_start = 0;
    while (label_start < domain.size())
    {
        const auto dot = domain.find('.', label_start);
        const auto label_end = dot == std::string_view::npos ? domain.size() : dot;
        const auto label = domain.substr(label_start, label_end - label_start);
        if (label.empty() || label.size() > 63 || label.front() == '-' || label.back() == '-' ||
            !std::all_of(label.begin(), label.end(), IsDomainLabelChar))
        {
            return false;
        }
        if (dot == std::string_view::npos)
        {
            break;
        }
        label_start = dot + 1;
    }

    const auto last_dot = domain.rfind('.');
    const auto tld = domain.substr(last_dot + 1);
    if (tld.size() < 2)
    {
        return false;
    }
    return std::all_of(tld.begin(),
                       tld.end(),
                       [](char ch)
                       {
                           return std::isalpha(static_cast<unsigned char>(ch));
                       });
}

bool IsValidAuthRefreshTokenShape(std::string_view refresh_token)
{
    if (refresh_token.size() > kMaxRefreshTokenLength)
    {
        return false;
    }
    const auto separator = refresh_token.find('.');
    if (separator == std::string_view::npos || refresh_token.find('.', separator + 1) != std::string_view::npos)
    {
        return false;
    }
    const auto selector = refresh_token.substr(0, separator);
    const auto verifier = refresh_token.substr(separator + 1);
    return selector.size() == kRefreshTokenSelectorLength && verifier.size() == kRefreshTokenVerifierLength &&
           IsLowerHex(selector) && IsLowerHex(verifier);
}

std::string AuthRefreshTokenRateLimitSubject(std::string_view refresh_token)
{
    if (!IsValidAuthRefreshTokenShape(refresh_token))
    {
        return "";
    }
    return std::string(refresh_token.substr(0, refresh_token.find('.')));
}

std::string ExtractWebRefreshTokenCookie(std::string_view cookie_header)
{
    while (!cookie_header.empty())
    {
        const auto separator = cookie_header.find(';');
        const auto entry = TrimAscii(cookie_header.substr(0, separator));
        const auto equals = entry.find('=');
        if (equals != std::string_view::npos && TrimAscii(entry.substr(0, equals)) == kWebRefreshCookieName)
        {
            const auto value = TrimAscii(entry.substr(equals + 1));
            return IsValidAuthRefreshTokenShape(value) ? std::string(value) : std::string{};
        }
        if (separator == std::string_view::npos)
        {
            break;
        }
        cookie_header.remove_prefix(separator + 1);
    }
    return {};
}

std::string BuildWebRefreshTokenCookie(std::string_view refresh_token, int ttl_seconds)
{
    if (!IsValidAuthRefreshTokenShape(refresh_token) || ttl_seconds <= 0)
    {
        return {};
    }
    return std::string(kWebRefreshCookieName) + "=" + std::string(refresh_token) +
           "; Path=/; Max-Age=" + std::to_string(ttl_seconds) + "; HttpOnly; Secure; SameSite=Strict";
}

std::string ClearWebRefreshTokenCookie()
{
    return std::string(kWebRefreshCookieName) + "=; Path=/; Max-Age=0; HttpOnly; Secure; SameSite=Strict";
}

AuthInputValidationResult ValidateAuthEmailRequest(const AuthEmailRequestDto& request)
{
    if (const auto result = ValidateStringField(request.email, AuthInputField::Email, true); !result.ok())
    {
        return result;
    }
    if (!IsValidAuthEmail(request.email))
    {
        return InvalidEmail();
    }
    return {};
}

AuthInputValidationResult ValidateAuthResetPasswordRequest(const AuthResetPasswordRequestDto& request)
{
    if (const auto result = ValidateAuthEmailRequest(AuthEmailRequestDto{.email = request.email}); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.user, AuthInputField::User, true); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.passwd, AuthInputField::Passwd, true); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.varifycode, AuthInputField::VarifyCode, true); !result.ok())
    {
        return result;
    }
    return {};
}

AuthInputValidationResult ValidateAuthRegisterRequest(const AuthRegisterRequestDto& request)
{
    if (const auto result = ValidateAuthEmailRequest(AuthEmailRequestDto{.email = request.email}); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.user, AuthInputField::User, true); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.passwd, AuthInputField::Passwd, true); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.confirm, AuthInputField::Confirm, true); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.icon, AuthInputField::Icon, false); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.varifycode, AuthInputField::VarifyCode, true); !result.ok())
    {
        return result;
    }
    return {};
}

AuthInputValidationResult ValidateAuthLoginRequest(const AuthLoginRequestDto& request)
{
    if (const auto result = ValidateAuthEmailRequest(AuthEmailRequestDto{.email = request.email}); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.passwd, AuthInputField::Passwd, true); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.client_ver, AuthInputField::ClientVer, false); !result.ok())
    {
        return result;
    }
    return {};
}

AuthInputValidationResult ValidateAuthRefreshRequest(const AuthRefreshRequestDto& request)
{
    if (const auto result = ValidateStringField(request.refresh_token, AuthInputField::RefreshToken, true);
        !result.ok())
    {
        return result;
    }
    if (!IsValidAuthRefreshTokenShape(request.refresh_token))
    {
        return InvalidRefreshToken();
    }
    if (const auto result = ValidateStringField(request.client_ver, AuthInputField::ClientVer, false); !result.ok())
    {
        return result;
    }
    return {};
}

AuthInputValidationResult ValidateAuthLogoutRequest(const AuthLogoutRequestDto& request)
{
    if (!request.refresh_token.empty())
    {
        if (const auto result = ValidateStringField(request.refresh_token, AuthInputField::RefreshToken, false);
            !result.ok())
        {
            return result;
        }
        if (!IsValidAuthRefreshTokenShape(request.refresh_token))
        {
            return InvalidRefreshToken();
        }
    }
    else if (!request.all_devices)
    {
        return Missing(AuthInputField::RefreshToken);
    }
    if (const auto result = ValidateStringField(request.client_ver, AuthInputField::ClientVer, false); !result.ok())
    {
        return result;
    }
    return {};
}

AuthInputValidationResult ValidateProfileUpdateRequest(const ProfileUpdateRequestDto& request)
{
    if (const auto result = ValidateStringField(request.name, AuthInputField::User, false); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.nick, AuthInputField::Nick, true); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.desc, AuthInputField::Desc, false); !result.ok())
    {
        return result;
    }
    if (const auto result = ValidateStringField(request.icon, AuthInputField::Icon, false); !result.ok())
    {
        return result;
    }
    return {};
}

AuthEmailRequestDto AuthEmailRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AuthEmailRequestDto request;
    request.email = memochat::json::glaze_safe_get<std::string>(root, "email", "");
    return request;
}

AuthResetPasswordRequestDto AuthResetPasswordRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AuthResetPasswordRequestDto request;
    request.email = memochat::json::glaze_safe_get<std::string>(root, "email", "");
    request.user = memochat::json::glaze_safe_get<std::string>(root, "user", "");
    request.passwd = memochat::json::glaze_safe_get<std::string>(root, "passwd", "");
    request.varifycode = memochat::json::glaze_safe_get<std::string>(root, "varifycode", "");
    return request;
}

AuthRegisterRequestDto AuthRegisterRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AuthRegisterRequestDto request;
    request.email = memochat::json::glaze_safe_get<std::string>(root, "email", "");
    request.user = memochat::json::glaze_safe_get<std::string>(root, "user", "");
    request.passwd = memochat::json::glaze_safe_get<std::string>(root, "passwd", "");
    request.confirm = memochat::json::glaze_safe_get<std::string>(root, "confirm", "");
    request.icon = memochat::json::glaze_safe_get<std::string>(root, "icon", "");
    request.varifycode = memochat::json::glaze_safe_get<std::string>(root, "varifycode", "");
    request.sex = memochat::json::glaze_safe_get<int>(root, "sex", 0);
    return request;
}

AuthLoginRequestDto AuthLoginRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AuthLoginRequestDto request;
    request.email = memochat::json::glaze_safe_get<std::string>(root, "email", "");
    request.passwd = memochat::json::glaze_safe_get<std::string>(root, "passwd", "");
    request.client_ver = memochat::json::glaze_safe_get<std::string>(root, "client_ver", "");
    return request;
}

AuthRefreshRequestDto AuthRefreshRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AuthRefreshRequestDto request;
    request.refresh_token = memochat::json::glaze_safe_get<std::string>(root, "refresh_token", "");
    request.client_ver = memochat::json::glaze_safe_get<std::string>(root, "client_ver", "");
    return request;
}

AuthLogoutRequestDto AuthLogoutRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AuthLogoutRequestDto request;
    request.refresh_token = memochat::json::glaze_safe_get<std::string>(root, "refresh_token", "");
    request.client_ver = memochat::json::glaze_safe_get<std::string>(root, "client_ver", "");
    request.all_devices = memochat::json::glaze_safe_get<bool>(root, "all_devices", false);
    return request;
}

ProfileUpdateRequestDto ProfileUpdateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ProfileUpdateRequestDto request;
    request.name = memochat::json::glaze_safe_get<std::string>(root, "name", "");
    request.nick = memochat::json::glaze_safe_get<std::string>(root, "nick", "");
    request.desc = memochat::json::glaze_safe_get<std::string>(root, "desc", "");
    request.icon = memochat::json::glaze_safe_get<std::string>(root, "icon", "");
    return request;
}

GetUserInfoRequestDto GetUserInfoRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    (void) root;
    GetUserInfoRequestDto request;
    return request;
}

bool HasAuthEmailRequiredFields(const memochat::json::JsonValue& root)
{
    return memochat::account::auth_public::modules::HasAuthEmailRequiredShape(
        memochat::json::glaze_has_key(root, "email"));
}

bool HasProfileUpdateRequiredFields(const memochat::json::JsonValue& root)
{
    return memochat::account::auth_public::modules::HasProfileUpdateRequiredShape(
        memochat::json::glaze_has_key(root, "nick"),
        memochat::json::glaze_has_key(root, "desc"),
        memochat::json::glaze_has_key(root, "icon"));
}

bool DecodeAuthEmailRequest(std::string_view body,
                            AuthEmailRequestDto* out,
                            memochat::json::JsonValue* parsed_root,
                            std::string* error_out)
{
    return DecodeAuthPublicRequest(body, out, parsed_root, error_out, AuthEmailRequestFromJsonValue);
}

bool DecodeAuthResetPasswordRequest(std::string_view body,
                                    AuthResetPasswordRequestDto* out,
                                    memochat::json::JsonValue* parsed_root,
                                    std::string* error_out)
{
    return DecodeAuthPublicRequest(body, out, parsed_root, error_out, AuthResetPasswordRequestFromJsonValue);
}

bool DecodeAuthRegisterRequest(std::string_view body,
                               AuthRegisterRequestDto* out,
                               memochat::json::JsonValue* parsed_root,
                               std::string* error_out)
{
    return DecodeAuthPublicRequest(body, out, parsed_root, error_out, AuthRegisterRequestFromJsonValue);
}

bool DecodeAuthLoginRequest(std::string_view body,
                            AuthLoginRequestDto* out,
                            memochat::json::JsonValue* parsed_root,
                            std::string* error_out)
{
    return DecodeAuthPublicRequest(body, out, parsed_root, error_out, AuthLoginRequestFromJsonValue);
}

bool DecodeAuthRefreshRequest(std::string_view body,
                              AuthRefreshRequestDto* out,
                              memochat::json::JsonValue* parsed_root,
                              std::string* error_out)
{
    return DecodeAuthPublicRequest(body, out, parsed_root, error_out, AuthRefreshRequestFromJsonValue);
}

bool DecodeAuthLogoutRequest(std::string_view body,
                             AuthLogoutRequestDto* out,
                             memochat::json::JsonValue* parsed_root,
                             std::string* error_out)
{
    return DecodeAuthPublicRequest(body, out, parsed_root, error_out, AuthLogoutRequestFromJsonValue);
}

bool DecodeProfileUpdateRequest(std::string_view body,
                                ProfileUpdateRequestDto* out,
                                memochat::json::JsonValue* parsed_root,
                                std::string* error_out)
{
    return DecodeAuthPublicRequest(body, out, parsed_root, error_out, ProfileUpdateRequestFromJsonValue);
}

bool DecodeGetUserInfoRequest(std::string_view body,
                              GetUserInfoRequestDto* out,
                              memochat::json::JsonValue* parsed_root,
                              std::string* error_out)
{
    return DecodeAuthPublicRequest(body, out, parsed_root, error_out, GetUserInfoRequestFromJsonValue);
}

memochat::json::JsonValue AuthResetPasswordResponseToJsonValue(const AuthResetPasswordResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AuthRegisterResponseToJsonValue(const AuthRegisterResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue ProfileUpdateResponseToJsonValue(const ProfileUpdateResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue UserInfoResponseToJsonValue(const UserInfoResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AuthLogoutResponseToJsonValue(const AuthLogoutResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AuthLoginUserProfileToJsonValue(const AuthLoginUserProfileDto& profile)
{
    return TypedJsonToJsonValue(profile);
}

memochat::json::JsonValue AuthChatEndpointToJsonValue(const AuthChatEndpointDto& endpoint)
{
    return TypedJsonToJsonValue(endpoint);
}

memochat::json::JsonValue AuthLoginStageMetricsToJsonValue(const AuthLoginStageMetricsDto& metrics)
{
    return TypedJsonToJsonValue(metrics);
}

} // namespace gateauthsupport

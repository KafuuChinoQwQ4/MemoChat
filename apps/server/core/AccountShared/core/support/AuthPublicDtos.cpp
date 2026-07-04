#include "AuthPublicDtos.hpp"

#include "json/TypedJsonCodec.hpp"

import memochat.account.auth_public_dto_algorithms;

#include <exception>

namespace
{

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
    try
    {
        return memochat::json::WriteTypedJson(value, out, error_out);
    }
    catch (const std::exception& e)
    {
        if (error_out != nullptr)
        {
            *error_out = e.what();
        }
        return false;
    }
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

} // namespace

namespace gateauthsupport
{

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
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    request.refresh_token = memochat::json::glaze_safe_get<std::string>(root, "refresh_token", "");
    request.client_ver = memochat::json::glaze_safe_get<std::string>(root, "client_ver", "");
    request.all_devices = memochat::json::glaze_safe_get<bool>(root, "all_devices", false);
    return request;
}

ProfileUpdateRequestDto ProfileUpdateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    ProfileUpdateRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.token = memochat::json::glaze_safe_get<std::string>(root, "token", "");
    request.name = memochat::json::glaze_safe_get<std::string>(root, "name", "");
    request.nick = memochat::json::glaze_safe_get<std::string>(root, "nick", "");
    request.desc = memochat::json::glaze_safe_get<std::string>(root, "desc", "");
    request.icon = memochat::json::glaze_safe_get<std::string>(root, "icon", "");
    return request;
}

GetUserInfoRequestDto GetUserInfoRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    GetUserInfoRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
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
        memochat::json::glaze_has_key(root, "uid"),
        memochat::json::glaze_has_key(root, "token"),
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

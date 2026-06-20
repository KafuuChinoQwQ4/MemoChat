#pragma once

#include "json/GlazeCompat.h"

#include <string>
#include <string_view>

namespace gateauthsupport
{

struct AuthEmailRequestDto
{
    std::string email;
};

struct AuthResetPasswordRequestDto
{
    std::string email;
    std::string user;
    std::string passwd;
    std::string varifycode;
};

struct AuthRegisterRequestDto
{
    std::string email;
    std::string user;
    std::string passwd;
    std::string confirm;
    std::string icon;
    std::string varifycode;
    int sex = 0;
};

struct AuthLoginRequestDto
{
    std::string email;
    std::string passwd;
    std::string client_ver;
};

struct ProfileUpdateRequestDto
{
    int uid = 0;
    std::string name;
    std::string nick;
    std::string desc;
    std::string icon;
};

struct GetUserInfoRequestDto
{
    int uid = 0;
};

struct AuthResetPasswordResponseDto
{
    int error = 0;
    std::string email;
    std::string user;
    std::string passwd;
    std::string varifycode;
};

struct AuthRegisterResponseDto
{
    int error = 0;
    int uid = 0;
    std::string user_id;
    std::string email;
    std::string user;
    std::string passwd;
    std::string confirm;
    std::string icon;
    std::string varifycode;
};

struct ProfileUpdateResponseDto
{
    int error = 0;
    int uid = 0;
    std::string name;
    std::string nick;
    std::string desc;
    std::string icon;
};

struct UserInfoResponseDto
{
    int error = 0;
    int uid = 0;
    std::string user_id;
    std::string name;
    std::string email;
    std::string nick;
    std::string icon;
    std::string desc;
    int sex = 0;
};

// Nested login-response sub-objects (HandleUserLogin). Field order matches the
// current inline wire output exactly; serialized via TypedJsonToJsonValue (Glaze,
// declaration order). No conditional fields — every field is always emitted.
struct AuthLoginUserProfileDto
{
    int uid = 0;
    std::string user_id;
    std::string name;
    std::string nick;
    std::string icon;
    std::string desc;
    std::string email;
    int sex = 0;
};

struct AuthChatEndpointDto
{
    std::string transport;
    std::string host;
    std::string port;
    std::string server_name;
    int priority = 0;
};

struct AuthLoginStageMetricsDto
{
    int64_t mysql_check_pwd_ms = 0;
    int64_t route_select_ms = 0;
    int64_t ticket_issue_ms = 0;
    int64_t user_login_total_ms = 0;
    std::string route_source;
    std::string status_route_detail;
};

AuthEmailRequestDto AuthEmailRequestFromJsonValue(const memochat::json::JsonValue& root);
AuthResetPasswordRequestDto AuthResetPasswordRequestFromJsonValue(const memochat::json::JsonValue& root);
AuthRegisterRequestDto AuthRegisterRequestFromJsonValue(const memochat::json::JsonValue& root);
AuthLoginRequestDto AuthLoginRequestFromJsonValue(const memochat::json::JsonValue& root);
ProfileUpdateRequestDto ProfileUpdateRequestFromJsonValue(const memochat::json::JsonValue& root);
GetUserInfoRequestDto GetUserInfoRequestFromJsonValue(const memochat::json::JsonValue& root);

bool HasAuthEmailRequiredFields(const memochat::json::JsonValue& root);
bool HasProfileUpdateRequiredFields(const memochat::json::JsonValue& root);

bool DecodeAuthEmailRequest(std::string_view body,
                            AuthEmailRequestDto* out,
                            memochat::json::JsonValue* parsed_root = nullptr,
                            std::string* error_out = nullptr);
bool DecodeAuthResetPasswordRequest(std::string_view body,
                                    AuthResetPasswordRequestDto* out,
                                    memochat::json::JsonValue* parsed_root = nullptr,
                                    std::string* error_out = nullptr);
bool DecodeAuthRegisterRequest(std::string_view body,
                               AuthRegisterRequestDto* out,
                               memochat::json::JsonValue* parsed_root = nullptr,
                               std::string* error_out = nullptr);
bool DecodeAuthLoginRequest(std::string_view body,
                            AuthLoginRequestDto* out,
                            memochat::json::JsonValue* parsed_root = nullptr,
                            std::string* error_out = nullptr);
bool DecodeProfileUpdateRequest(std::string_view body,
                                ProfileUpdateRequestDto* out,
                                memochat::json::JsonValue* parsed_root = nullptr,
                                std::string* error_out = nullptr);
bool DecodeGetUserInfoRequest(std::string_view body,
                              GetUserInfoRequestDto* out,
                              memochat::json::JsonValue* parsed_root = nullptr,
                              std::string* error_out = nullptr);

memochat::json::JsonValue AuthResetPasswordResponseToJsonValue(const AuthResetPasswordResponseDto& response);
memochat::json::JsonValue AuthRegisterResponseToJsonValue(const AuthRegisterResponseDto& response);
memochat::json::JsonValue ProfileUpdateResponseToJsonValue(const ProfileUpdateResponseDto& response);
memochat::json::JsonValue UserInfoResponseToJsonValue(const UserInfoResponseDto& response);
memochat::json::JsonValue AuthLoginUserProfileToJsonValue(const AuthLoginUserProfileDto& profile);
memochat::json::JsonValue AuthChatEndpointToJsonValue(const AuthChatEndpointDto& endpoint);
memochat::json::JsonValue AuthLoginStageMetricsToJsonValue(const AuthLoginStageMetricsDto& metrics);

} // namespace gateauthsupport

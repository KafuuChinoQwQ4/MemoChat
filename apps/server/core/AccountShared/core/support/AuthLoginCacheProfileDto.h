#pragma once

#include "AuthUserInfo.h"

#include <string>
#include <string_view>

namespace gateauthsupport
{

struct AuthLoginCacheProfileDto
{
    int uid = 0;
    std::string pwd;
    std::string name;
    std::string email;
    std::string user_id;
    std::string nick;
    std::string icon;
    std::string desc;
    int sex = 0;
};

AuthLoginCacheProfileDto ToLoginCacheProfileDto(const UserInfo& user_info);
void ApplyLoginCacheProfileDto(const AuthLoginCacheProfileDto& dto, UserInfo& user_info);
bool EncodeLoginCacheProfile(const UserInfo& user_info, std::string* out, std::string* error_out = nullptr);
bool DecodeLoginCacheProfile(std::string_view body, AuthLoginCacheProfileDto* out, std::string* error_out = nullptr);
bool DecodeLoginCacheProfile(std::string_view body, UserInfo* out, std::string* error_out = nullptr);

} // namespace gateauthsupport

#include "AuthLoginCacheProfileDto.h"

#include "json/TypedJsonCodec.h"

namespace gateauthsupport
{

AuthLoginCacheProfileDto ToLoginCacheProfileDto(const UserInfo& user_info)
{
    AuthLoginCacheProfileDto dto;
    dto.uid = user_info.uid;
    dto.pwd = user_info.pwd;
    dto.name = user_info.name;
    dto.email = user_info.email;
    dto.user_id = user_info.user_id;
    dto.nick = user_info.nick;
    dto.icon = user_info.icon;
    dto.desc = user_info.desc;
    dto.sex = user_info.sex;
    return dto;
}

void ApplyLoginCacheProfileDto(const AuthLoginCacheProfileDto& dto, UserInfo& user_info)
{
    user_info.uid = dto.uid;
    user_info.pwd = dto.pwd;
    user_info.name = dto.name;
    user_info.email = dto.email;
    user_info.user_id = dto.user_id;
    user_info.nick = dto.nick;
    user_info.icon = dto.icon;
    user_info.desc = dto.desc;
    user_info.sex = dto.sex;
}

bool EncodeLoginCacheProfile(const UserInfo& user_info, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(ToLoginCacheProfileDto(user_info), out, error_out);
}

bool DecodeLoginCacheProfile(std::string_view body, AuthLoginCacheProfileDto* out, std::string* error_out)
{
    if (!memochat::json::ReadTypedJson(body, out, error_out))
    {
        return false;
    }
    return out != nullptr && out->uid > 0 && !out->pwd.empty();
}

bool DecodeLoginCacheProfile(std::string_view body, UserInfo* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }

    AuthLoginCacheProfileDto dto;
    if (!DecodeLoginCacheProfile(body, &dto, error_out))
    {
        return false;
    }
    ApplyLoginCacheProfileDto(dto, *out);
    return true;
}

} // namespace gateauthsupport


#include "ChatUserProfileDto.h"

#include "data.h"
#include "json/TypedJsonCodec.h"

#include <exception>

namespace
{

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

} // namespace

namespace chatusersupport
{

ChatUserProfileDto ChatUserProfileFromUserInfo(const UserInfo& user_info)
{
    ChatUserProfileDto profile;
    profile.uid = user_info.uid;
    profile.user_id = user_info.user_id;
    profile.pwd = user_info.pwd;
    profile.name = user_info.name;
    profile.email = user_info.email;
    profile.nick = user_info.nick;
    profile.desc = user_info.desc;
    profile.sex = user_info.sex;
    profile.icon = user_info.icon;
    return profile;
}

void FillUserInfoFromChatUserProfile(const ChatUserProfileDto& profile, UserInfo& user_info)
{
    user_info.uid = profile.uid;
    user_info.user_id = profile.user_id;
    user_info.pwd = profile.pwd;
    user_info.name = profile.name;
    user_info.email = profile.email;
    user_info.nick = profile.nick;
    user_info.desc = profile.desc;
    user_info.sex = profile.sex;
    user_info.icon = profile.icon;
}

bool EncodeChatUserProfileCache(const ChatUserProfileDto& profile, std::string* out, std::string* error_out)
{
    return WriteTypedJsonNoThrow(profile, out, error_out);
}

bool DecodeChatUserProfileCache(std::string_view body, ChatUserProfileDto* out, std::string* error_out)
{
    try
    {
        return memochat::json::ReadTypedJson(body, out, error_out);
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

memochat::json::JsonValue
ChatUserProfileToJsonValue(const ChatUserProfileDto& profile, bool include_pwd, bool include_icon)
{
    memochat::json::JsonValue out(memochat::json::object_t{});
    AppendChatUserProfileToJsonValue(profile, out, include_pwd, include_icon);
    return out;
}

void AppendChatUserProfileToJsonValue(const ChatUserProfileDto& profile,
                                      memochat::json::JsonValue& out,
                                      bool include_pwd,
                                      bool include_icon)
{
    out["uid"] = profile.uid;
    out["user_id"] = profile.user_id;
    if (include_pwd)
    {
        out["pwd"] = profile.pwd;
    }
    out["name"] = profile.name;
    out["email"] = profile.email;
    out["nick"] = profile.nick;
    out["desc"] = profile.desc;
    out["sex"] = profile.sex;
    if (include_icon)
    {
        out["icon"] = profile.icon;
    }
}

} // namespace chatusersupport

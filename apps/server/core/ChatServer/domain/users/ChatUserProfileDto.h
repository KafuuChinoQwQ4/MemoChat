#pragma once

#include "json/GlazeCompat.h"

#include <string>
#include <string_view>

struct UserInfo;

namespace chatusersupport
{

struct ChatUserProfileDto
{
    int uid = 0;
    std::string user_id;
    std::string pwd;
    std::string name;
    std::string email;
    std::string nick;
    std::string desc;
    int sex = 0;
    std::string icon;
};

ChatUserProfileDto ChatUserProfileFromUserInfo(const UserInfo& user_info);
void FillUserInfoFromChatUserProfile(const ChatUserProfileDto& profile, UserInfo& user_info);

bool EncodeChatUserProfileCache(const ChatUserProfileDto& profile, std::string* out, std::string* error_out = nullptr);
bool DecodeChatUserProfileCache(std::string_view body, ChatUserProfileDto* out, std::string* error_out = nullptr);

memochat::json::JsonValue
ChatUserProfileToJsonValue(const ChatUserProfileDto& profile, bool include_pwd, bool include_icon);
void AppendChatUserProfileToJsonValue(const ChatUserProfileDto& profile,
                                      memochat::json::JsonValue& out,
                                      bool include_pwd,
                                      bool include_icon);

} // namespace chatusersupport

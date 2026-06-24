#include "ChatUserSupport.h"

#include "ChatUserProfileDto.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "data.h"

#include <cctype>
#include <iostream>

namespace chatusersupport
{

bool IsPureDigit(const std::string& str)
{
    for (char c : str)
    {
        if (!std::isdigit(static_cast<unsigned char>(c)))
        {
            return false;
        }
    }
    return true;
}

void GetUserByUid(const std::string& uid_str, memochat::json::JsonValue& rtvalue)
{
    rtvalue["error"] = ErrorCodes::Success;
    const std::string base_key = USER_BASE_INFO + uid_str;

    auto user_info = PostgresMgr::GetInstance()->GetUser(std::stoi(uid_str));
    if (!user_info)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    const ChatUserProfileDto profile = ChatUserProfileFromUserInfo(*user_info);
    std::string cache_body;
    if (EncodeChatUserProfileCache(profile, &cache_body))
    {
        RedisMgr::GetInstance()->Set(base_key, cache_body);
    }

    AppendChatUserProfileToJsonValue(profile, rtvalue, false, true);
}

void GetUserByName(const std::string& name, memochat::json::JsonValue& rtvalue)
{
    rtvalue["error"] = ErrorCodes::Success;
    const std::string base_key = NAME_INFO + name;

    std::string info_str;
    if (RedisMgr::GetInstance()->Get(base_key, info_str))
    {
        ChatUserProfileDto profile;
        DecodeChatUserProfileCache(info_str, &profile);
        AppendChatUserProfileToJsonValue(profile, rtvalue, false, false);
        return;
    }

    auto user_info = PostgresMgr::GetInstance()->GetUser(name);
    if (!user_info)
    {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    const ChatUserProfileDto profile = ChatUserProfileFromUserInfo(*user_info);
    std::string cache_body;
    if (EncodeChatUserProfileCache(profile, &cache_body))
    {
        RedisMgr::GetInstance()->Set(base_key, cache_body);
    }

    AppendChatUserProfileToJsonValue(profile, rtvalue, true, false);
}

bool GetBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
    std::string info_str;
    if (RedisMgr::GetInstance()->Get(base_key, info_str))
    {
        ChatUserProfileDto profile;
        DecodeChatUserProfileCache(info_str, &profile);
        if (!profile.user_id.empty())
        {
            FillUserInfoFromChatUserProfile(profile, *userinfo);
            return true;
        }
    }

    auto user_info = PostgresMgr::GetInstance()->GetUser(uid);
    if (!user_info)
    {
        return false;
    }
    userinfo = user_info;

    const ChatUserProfileDto profile = ChatUserProfileFromUserInfo(*userinfo);
    std::string cache_body;
    if (EncodeChatUserProfileCache(profile, &cache_body))
    {
        RedisMgr::GetInstance()->Set(base_key, cache_body);
    }
    return true;
}

bool GetFriendApplyInfo(int to_uid, std::vector<std::shared_ptr<ApplyInfo>>& list)
{
    return PostgresMgr::GetInstance()->GetApplyList(to_uid, list, 0, 10);
}

bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list)
{
    return PostgresMgr::GetInstance()->GetFriendList(self_id, user_list);
}

} // namespace chatusersupport

#include "ChatUserSupport.hpp"

#include "ChatUserProfileDto.hpp"
#include "PostgresMgr.hpp"
#include "RedisMgr.hpp"
#include "data.hpp"

#include <iostream>

import memochat.chat.user_support_algorithms;

namespace user_support_modules = memochat::chat::user_support::modules;

namespace chatusersupport
{

bool IsPureDigit(const std::string& str)
{
    for (char c : str)
    {
        if (!user_support_modules::IsAsciiDigit(static_cast<unsigned char>(c)))
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
    if (user_support_modules::ShouldReportMissingUser(user_info != nullptr))
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

    AppendChatUserProfileToJsonValue(profile, rtvalue, true);
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
        AppendChatUserProfileToJsonValue(profile, rtvalue, true);
        return;
    }

    auto user_info = PostgresMgr::GetInstance()->GetUser(name);
    if (user_support_modules::ShouldReportMissingUser(user_info != nullptr))
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

    AppendChatUserProfileToJsonValue(profile, rtvalue, true);
}

bool GetBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
    std::string info_str;
    const bool cache_hit = RedisMgr::GetInstance()->Get(base_key, info_str);
    if (cache_hit)
    {
        ChatUserProfileDto profile;
        DecodeChatUserProfileCache(info_str, &profile);
        if (user_support_modules::ShouldUseCachedProfile(cache_hit, profile.user_id.empty()))
        {
            FillUserInfoFromChatUserProfile(profile, *userinfo);
            return true;
        }
    }

    auto user_info = PostgresMgr::GetInstance()->GetUser(uid);
    if (user_support_modules::ShouldReportMissingUser(user_info != nullptr))
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
    return PostgresMgr::GetInstance()->GetApplyList(to_uid,
                                                    list,
                                                    user_support_modules::FriendApplyOffset(),
                                                    user_support_modules::FriendApplyLimit());
}

bool GetFriendList(int self_id, std::vector<std::shared_ptr<UserInfo>>& user_list)
{
    return PostgresMgr::GetInstance()->GetFriendList(self_id, user_list);
}

} // namespace chatusersupport

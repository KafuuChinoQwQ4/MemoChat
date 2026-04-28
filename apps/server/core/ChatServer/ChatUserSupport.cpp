#include "ChatUserSupport.h"

#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "data.h"

#include <cctype>
#include <iostream>
#include "json/GlazeCompat.h"

namespace chatusersupport {

bool IsPureDigit(const std::string& str)
{
    for (char c : str) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
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
    if (!user_info) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    memochat::json::JsonValue redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["user_id"] = user_info->user_id;
    redis_root["pwd"] = user_info->pwd;
    redis_root["name"] = user_info->name;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    redis_root["icon"] = user_info->icon;
    RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());

    rtvalue["uid"] = user_info->uid;
    rtvalue["user_id"] = user_info->user_id;
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
    rtvalue["icon"] = user_info->icon;
}

void GetUserByName(const std::string& name, memochat::json::JsonValue& rtvalue)
{
    rtvalue["error"] = ErrorCodes::Success;
    const std::string base_key = NAME_INFO + name;

    std::string info_str;
    if (RedisMgr::GetInstance()->Get(base_key, info_str)) {
        memochat::json::JsonReader reader;
        memochat::json::JsonValue root;
        reader.parse(info_str, root);
        rtvalue["uid"] = root["uid"].asInt();
        rtvalue["user_id"] = root["user_id"].asString();
        rtvalue["name"] = root["name"].asString();
        rtvalue["email"] = root["email"].asString();
        rtvalue["nick"] = root["nick"].asString();
        rtvalue["desc"] = root["desc"].asString();
        rtvalue["sex"] = root["sex"].asInt();
        return;
    }

    auto user_info = PostgresMgr::GetInstance()->GetUser(name);
    if (!user_info) {
        rtvalue["error"] = ErrorCodes::UidInvalid;
        return;
    }

    memochat::json::JsonValue redis_root;
    redis_root["uid"] = user_info->uid;
    redis_root["user_id"] = user_info->user_id;
    redis_root["pwd"] = user_info->pwd;
    redis_root["name"] = user_info->name;
    redis_root["email"] = user_info->email;
    redis_root["nick"] = user_info->nick;
    redis_root["desc"] = user_info->desc;
    redis_root["sex"] = user_info->sex;
    RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());

    rtvalue["uid"] = user_info->uid;
    rtvalue["user_id"] = user_info->user_id;
    rtvalue["pwd"] = user_info->pwd;
    rtvalue["name"] = user_info->name;
    rtvalue["email"] = user_info->email;
    rtvalue["nick"] = user_info->nick;
    rtvalue["desc"] = user_info->desc;
    rtvalue["sex"] = user_info->sex;
}

bool GetBaseInfo(const std::string& base_key, int uid, std::shared_ptr<UserInfo>& userinfo)
{
    std::string info_str;
    if (RedisMgr::GetInstance()->Get(base_key, info_str)) {
        memochat::json::JsonReader reader;
        memochat::json::JsonValue root;
        reader.parse(info_str, root);
        userinfo->uid = root["uid"].asInt();
        userinfo->user_id = root["user_id"].asString();
        userinfo->name = root["name"].asString();
        userinfo->pwd = root["pwd"].asString();
        userinfo->email = root["email"].asString();
        userinfo->nick = root["nick"].asString();
        userinfo->desc = root["desc"].asString();
        userinfo->sex = root["sex"].asInt();
        userinfo->icon = root["icon"].asString();
        return true;
    }

    auto user_info = PostgresMgr::GetInstance()->GetUser(uid);
    if (!user_info) {
        return false;
    }
    userinfo = user_info;

    memochat::json::JsonValue redis_root;
    redis_root["uid"] = uid;
    redis_root["user_id"] = userinfo->user_id;
    redis_root["pwd"] = userinfo->pwd;
    redis_root["name"] = userinfo->name;
    redis_root["email"] = userinfo->email;
    redis_root["nick"] = userinfo->nick;
    redis_root["desc"] = userinfo->desc;
    redis_root["sex"] = userinfo->sex;
    redis_root["icon"] = userinfo->icon;
    RedisMgr::GetInstance()->Set(base_key, redis_root.toStyledString());
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

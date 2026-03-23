#include "WinCompat.h"
#include "DrogonProfileSupport.h"
#include "../GateServerCore/PostgresMgr.h"
#include "../GateServerCore/RedisMgr.h"
#include "../GateServerCore/const.h"
#include "../GateServerCore/AuthLoginSupport.h"
#include "logging/Logger.h"

namespace DrogonProfileSupport {

bool ParseJsonBody(std::string_view body_sv, Json::Value& root) {
    std::string body_str(body_sv);
    Json::Reader reader;
    return reader.parse(body_str, root) && root.isObject();
}

Json::Value MakeError(int error_code, const std::string& message) {
    Json::Value resp;
    resp["error"] = error_code;
    if (!message.empty()) {
        resp["message"] = message;
    }
    return resp;
}

ProfileResult HandleUserUpdateProfile(const Json::Value& req) {
    ProfileResult result;
    if (!req.isMember("uid") || !req.isMember("nick") ||
        !req.isMember("desc") || !req.isMember("icon")) {
        result.error = 1;  // Error_Json equivalent
        result.message = "missing required fields";
        return result;
    }

    const auto uid = req["uid"].asInt();
    const auto name = req.get("name", "").asString();
    const auto nick = req["nick"].asString();
    const auto desc = req["desc"].asString();
    const auto icon = req["icon"].asString();

    if (uid <= 0 || nick.empty()) {
        result.error = 1;
        result.message = "invalid uid or nick";
        return result;
    }

    if (!PostgresMgr::GetInstance()->UpdateUserProfile(uid, nick, desc, icon)) {
        result.error = 1;
        result.message = "profile update failed";
        return result;
    }

    RedisMgr::GetInstance()->Del("ubaseinfo_" + std::to_string(uid));
    if (!name.empty()) {
        RedisMgr::GetInstance()->Del("nameinfo_" + name);
    }
    gateauthsupport::InvalidateLoginCacheByUid(uid);

    result.error = 0;  // Success
    result.data["uid"] = uid;
    result.data["name"] = name;
    result.data["nick"] = nick;
    result.data["desc"] = desc;
    result.data["icon"] = icon;

    memolog::LogInfo("drogon.user_update_profile",
        "user profile updated via Drogon handler",
        {{"uid", std::to_string(uid)}});
    return result;
}

ProfileResult HandleGetUserInfo(int uid) {
    ProfileResult result;
    if (uid <= 0) {
        result.error = 1;
        result.message = "invalid uid";
        return result;
    }
    ::UserInfo user_info;
    if (!PostgresMgr::GetInstance()->GetUserInfo(uid, user_info)) {
        result.error = 1;
        result.message = "user not found";
        return result;
    }
    result.error = 0;  // Success
    result.data["uid"] = user_info.uid;
    result.data["name"] = user_info.name;
    result.data["email"] = user_info.email;
    result.data["nick"] = user_info.nick;
    result.data["icon"] = user_info.icon;
    result.data["desc"] = user_info.desc;
    result.data["sex"] = user_info.sex;
    return result;
}

}  // namespace DrogonProfileSupport

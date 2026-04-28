#include "Http2ProfileSupport.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include "AuthLoginSupport.h"
#include "logging/Logger.h"
#include "json/GlazeCompat.h"

namespace Http2ProfileSupport {

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root) {
    std::string body_str(body_sv);
    return memochat::json::glaze_parse(root, body_str) && memochat::json::glaze_is_object(root);
}

memochat::json::JsonValue MakeError(int error_code, const std::string& message) {
    memochat::json::JsonValue resp;
    resp["error"] = error_code;
    if (!message.empty()) {
        resp["message"] = message;
    }
    return resp;
}

ProfileResult HandleUserUpdateProfile(const memochat::json::JsonValue& req) {
    ProfileResult result;
    if (!memochat::json::glaze_has_key(req, "uid") || !memochat::json::glaze_has_key(req, "nick") ||
        !memochat::json::glaze_has_key(req, "desc") || !memochat::json::glaze_has_key(req, "icon")) {
        result.error = 1;
        result.message = "missing required fields";
        return result;
    }

    const auto uid = memochat::json::glaze_safe_get<int>(req, "uid", 0);
    const auto name = memochat::json::glaze_safe_get<std::string>(req, "name", "");
    const auto nick = memochat::json::glaze_safe_get<std::string>(req, "nick", "");
    const auto desc = memochat::json::glaze_safe_get<std::string>(req, "desc", "");
    const auto icon = memochat::json::glaze_safe_get<std::string>(req, "icon", "");

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

    result.error = 0;
    result.data["uid"] = uid;
    result.data["name"] = name;
    result.data["nick"] = nick;
    result.data["desc"] = desc;
    result.data["icon"] = icon;

    memolog::LogInfo("http2.user_update_profile",
        "user profile updated via HTTP2 handler",
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
    result.error = 0;
    result.data["uid"] = user_info.uid;
    result.data["user_id"] = user_info.user_id;
    result.data["name"] = user_info.name;
    result.data["email"] = user_info.email;
    result.data["nick"] = user_info.nick;
    result.data["icon"] = user_info.icon;
    result.data["desc"] = user_info.desc;
    result.data["sex"] = user_info.sex;
    return result;
}

}  // namespace Http2ProfileSupport


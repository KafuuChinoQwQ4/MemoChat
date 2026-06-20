#include "Http2ProfileSupport.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include "AuthLoginSupport.h"
#include "AuthPublicDtos.h"
#include "logging/Logger.h"
#include "json/GlazeCompat.h"

namespace Http2ProfileSupport
{

bool ParseJsonBody(std::string_view body_sv, memochat::json::JsonValue& root)
{
    std::string body_str(body_sv);
    return memochat::json::glaze_parse(root, body_str) && memochat::json::glaze_is_object(root);
}

memochat::json::JsonValue MakeError(int error_code, const std::string& message)
{
    memochat::json::JsonValue resp;
    resp["error"] = error_code;
    if (!message.empty())
    {
        resp["message"] = message;
    }
    return resp;
}

ProfileResult HandleUserUpdateProfile(const memochat::json::JsonValue& req)
{
    ProfileResult result;
    if (!gateauthsupport::HasProfileUpdateRequiredFields(req))
    {
        result.error = 1;
        result.message = "missing required fields";
        return result;
    }

    const auto profile_request = gateauthsupport::ProfileUpdateRequestFromJsonValue(req);
    const auto uid = profile_request.uid;
    const auto name = profile_request.name;
    const auto nick = profile_request.nick;
    const auto desc = profile_request.desc;
    const auto icon = profile_request.icon;

    if (uid <= 0 || nick.empty())
    {
        result.error = 1;
        result.message = "invalid uid or nick";
        return result;
    }

    if (!PostgresMgr::GetInstance()->UpdateUserProfile(uid, nick, desc, icon))
    {
        result.error = 1;
        result.message = "profile update failed";
        return result;
    }

    RedisMgr::GetInstance()->Del("ubaseinfo_" + std::to_string(uid));
    if (!name.empty())
    {
        RedisMgr::GetInstance()->Del("nameinfo_" + name);
    }
    gateauthsupport::InvalidateLoginCacheByUid(uid);

    result.error = 0;
    gateauthsupport::ProfileUpdateResponseDto profile_response;
    profile_response.error = 0;
    profile_response.uid = uid;
    profile_response.name = name;
    profile_response.nick = nick;
    profile_response.desc = desc;
    profile_response.icon = icon;
    result.data = gateauthsupport::ProfileUpdateResponseToJsonValue(profile_response);

    memolog::LogInfo("http2.user_update_profile",
                     "user profile updated via HTTP2 handler",
                     {{"uid", std::to_string(uid)}});
    return result;
}

ProfileResult HandleGetUserInfo(int uid)
{
    ProfileResult result;
    if (uid <= 0)
    {
        result.error = 1;
        result.message = "invalid uid";
        return result;
    }
    ::UserInfo user_info;
    if (!PostgresMgr::GetInstance()->GetUserInfo(uid, user_info))
    {
        result.error = 1;
        result.message = "user not found";
        return result;
    }
    result.error = 0;
    gateauthsupport::UserInfoResponseDto user_info_response;
    user_info_response.error = 0;
    user_info_response.uid = user_info.uid;
    user_info_response.user_id = user_info.user_id;
    user_info_response.name = user_info.name;
    user_info_response.email = user_info.email;
    user_info_response.nick = user_info.nick;
    user_info_response.icon = user_info.icon;
    user_info_response.desc = user_info.desc;
    user_info_response.sex = user_info.sex;
    result.data = gateauthsupport::UserInfoResponseToJsonValue(user_info_response);
    return result;
}

} // namespace Http2ProfileSupport

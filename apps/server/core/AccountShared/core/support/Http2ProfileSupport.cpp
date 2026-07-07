#include "Http2ProfileSupport.hpp"
#include "PostgresMgr.hpp"
#include "RedisMgr.hpp"
#include "const.hpp"
#include "AuthLoginSupport.hpp"
#include "AuthPublicDtos.hpp"
#include "logging/Logger.hpp"
#include "json/GlazeCompat.hpp"
#include "support/UserTokenValidator.hpp"

import memochat.account.profile_support_algorithms;

namespace profile_algo = memochat::account::profile_support::modules;

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

ProfileResult HandleUserUpdateProfile(const memochat::json::JsonValue& req, const std::string& access_token)
{
    ProfileResult result;
    if (!gateauthsupport::HasProfileUpdateRequiredFields(req))
    {
        result.error = profile_algo::ProfileErrorCode();
        result.message = profile_algo::MissingRequiredFieldsMessage();
        return result;
    }

    const auto profile_request = gateauthsupport::ProfileUpdateRequestFromJsonValue(req);
    const auto name = profile_request.name;
    const auto nick = profile_request.nick;
    const auto desc = profile_request.desc;
    const auto icon = profile_request.icon;

    if (!gateauthsupport::ValidateProfileUpdateRequest(profile_request).ok())
    {
        result.error = profile_algo::ProfileErrorCode();
        result.message = profile_algo::InvalidUidOrNickMessage();
        return result;
    }
    // H-8 verified: uid is derived exclusively from the validated HTTP token via
    // ResolveUserIdFromToken; any uid field in the request DTO is never used to
    // drive database operations, so no IDOR vulnerability exists here.
    int uid = 0;
    if (!memochat::auth::ResolveUserIdFromToken(access_token, uid))
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid";
        return result;
    }

    if (!PostgresMgr::GetInstance()->UpdateUserProfile(uid, nick, desc, icon))
    {
        result.error = profile_algo::ProfileErrorCode();
        result.message = profile_algo::ProfileUpdateFailedMessage();
        return result;
    }

    RedisMgr::GetInstance()->Del(profile_algo::UserBaseInfoKeyPrefix() + std::to_string(uid));
    if (!name.empty())
    {
        RedisMgr::GetInstance()->Del(profile_algo::UserNameInfoKeyPrefix() + name);
    }
    gateauthsupport::InvalidateLoginCacheByUid(uid);

    result.error = profile_algo::ProfileSuccessCode();
    gateauthsupport::ProfileUpdateResponseDto profile_response;
    profile_response.error = profile_algo::ProfileSuccessCode();
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

ProfileResult HandleGetUserInfo(const std::string& token)
{
    ProfileResult result;
    if (token.empty())
    {
        result.error = profile_algo::ProfileErrorCode();
        result.message = profile_algo::InvalidUidMessage();
        return result;
    }
    // H-8 verified: uid is derived exclusively from the validated HTTP token via
    // ResolveUserIdFromToken; no uid is accepted from caller-supplied data,
    // so no IDOR vulnerability exists here.
    int uid = 0;
    if (!memochat::auth::ResolveUserIdFromToken(token, uid))
    {
        result.error = ErrorCodes::TokenInvalid;
        result.message = "token invalid";
        return result;
    }
    ::UserInfo user_info;
    if (!PostgresMgr::GetInstance()->GetUserInfo(uid, user_info))
    {
        result.error = profile_algo::ProfileErrorCode();
        result.message = profile_algo::UserNotFoundMessage();
        return result;
    }
    result.error = profile_algo::ProfileSuccessCode();
    gateauthsupport::UserInfoResponseDto user_info_response;
    user_info_response.error = profile_algo::ProfileSuccessCode();
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

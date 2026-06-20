#include "modules/profile/ProfileRouteModule.h"

#include "AuthLoginSupport.h"
#include "AuthPublicDtos.h"
#include "PostgresMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include "json/GlazeCompat.h"
#include "routing/RouteRegistry.h"

#include <string>

namespace memochat::gate::modules::profile
{
namespace
{

void WriteJson(memochat::gate::routing::GateResponse& response, const memochat::json::JsonValue& root)
{
    response.status = 200;
    response.content_type = "application/json";
    response.body = root.toStyledString();
}

bool HandleUserUpdateProfile(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    memochat::json::JsonValue src_root;
    gateauthsupport::ProfileUpdateRequestDto profile_request;
    if (!gateauthsupport::DecodeProfileUpdateRequest(request.body, &profile_request, &src_root))
    {
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    if (!gateauthsupport::HasProfileUpdateRequiredFields(src_root))
    {
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto uid = profile_request.uid;
    const auto name = profile_request.name;
    const auto nick = profile_request.nick;
    const auto desc = profile_request.desc;
    const auto icon = profile_request.icon;
    if (uid <= 0 || nick.empty())
    {
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    if (!PostgresMgr::GetInstance()->UpdateUserProfile(uid, nick, desc, icon))
    {
        root["error"] = ErrorCodes::ProfileUpFailed;
        WriteJson(response, root);
        return true;
    }

    RedisMgr::GetInstance()->Del("ubaseinfo_" + std::to_string(uid));
    if (!name.empty())
    {
        RedisMgr::GetInstance()->Del("nameinfo_" + name);
    }
    gateauthsupport::InvalidateLoginCacheByUid(uid);

    gateauthsupport::ProfileUpdateResponseDto profile_response;
    profile_response.error = ErrorCodes::Success;
    profile_response.uid = uid;
    profile_response.name = name;
    profile_response.nick = nick;
    profile_response.desc = desc;
    profile_response.icon = icon;
    root = gateauthsupport::ProfileUpdateResponseToJsonValue(profile_response);
    WriteJson(response, root);
    return true;
}

bool HandleGetUserInfo(const memochat::gate::routing::GateRequest& request,
                       memochat::gate::routing::GateResponse& response)
{
    memochat::json::JsonValue root;
    gateauthsupport::GetUserInfoRequestDto user_info_request;
    if (!gateauthsupport::DecodeGetUserInfoRequest(request.body, &user_info_request))
    {
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const int uid = user_info_request.uid;
    if (uid <= 0)
    {
        root["error"] = ErrorCodes::UidInvalid;
        WriteJson(response, root);
        return true;
    }

    UserInfo user_info;
    if (!PostgresMgr::GetInstance()->GetUserInfo(uid, user_info))
    {
        root["error"] = ErrorCodes::RPCFailed;
        WriteJson(response, root);
        return true;
    }

    gateauthsupport::UserInfoResponseDto user_info_response;
    user_info_response.error = ErrorCodes::Success;
    user_info_response.uid = user_info.uid;
    user_info_response.user_id = user_info.user_id;
    user_info_response.name = user_info.name;
    user_info_response.email = user_info.email;
    user_info_response.nick = user_info.nick;
    user_info_response.icon = user_info.icon;
    user_info_response.desc = user_info.desc;
    user_info_response.sex = user_info.sex;
    root = gateauthsupport::UserInfoResponseToJsonValue(user_info_response);
    WriteJson(response, root);
    return true;
}

} // namespace

void ProfileRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register("POST", "/user_update_profile", HandleUserUpdateProfile);
}

void ProfileRouteModule::RegisterUserInfoRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register("POST", "/get_user_info", HandleGetUserInfo);
}

} // namespace memochat::gate::modules::profile

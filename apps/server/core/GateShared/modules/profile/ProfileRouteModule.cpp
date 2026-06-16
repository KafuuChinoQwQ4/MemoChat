#include "modules/profile/ProfileRouteModule.h"

#include "AuthLoginSupport.h"
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
    memochat::json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    if (!isMember(src_root, "uid") || !isMember(src_root, "nick") || !isMember(src_root, "desc") ||
        !isMember(src_root, "icon"))
    {
        root["error"] = ErrorCodes::Error_Json;
        WriteJson(response, root);
        return true;
    }

    const auto uid = src_root["uid"].asInt();
    const auto name = src_root.get("name", "").asString();
    const auto nick = src_root["nick"].asString();
    const auto desc = src_root["desc"].asString();
    const auto icon = src_root["icon"].asString();
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

    root["error"] = ErrorCodes::Success;
    root["uid"] = uid;
    root["name"] = name;
    root["nick"] = nick;
    root["desc"] = desc;
    root["icon"] = icon;
    WriteJson(response, root);
    return true;
}

} // namespace

void ProfileRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register("POST", "/user_update_profile", HandleUserUpdateProfile);
}

} // namespace memochat::gate::modules::profile

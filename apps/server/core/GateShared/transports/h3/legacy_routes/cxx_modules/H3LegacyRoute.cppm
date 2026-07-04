export module memochat.gate.h3_legacy_route_algorithms;

export namespace memochat::gate::h3::legacy::modules
{
int GetRouteCount()
{
    return 4;
}

const char* GetRoutePathAt(int index)
{
    switch (index)
    {
    case 0:
        return "/healthz";
    case 1:
        return "/readyz";
    case 2:
        return "/upload_media_status";
    case 3:
        return "/media/download";
    default:
        return "";
    }
}

int PostRouteCount()
{
    return 24;
}

const char* PostRoutePathAt(int index)
{
    switch (index)
    {
    case 0:
        return "/get_varifycode";
    case 1:
        return "/user_register";
    case 2:
        return "/reset_pwd";
    case 3:
        return "/user_login";
    case 4:
        return "/user_update_profile";
    case 5:
        return "/get_user_info";
    case 6:
        return "/api/call/token";
    case 7:
        return "/api/call/start";
    case 8:
        return "/api/call/accept";
    case 9:
        return "/api/call/reject";
    case 10:
        return "/api/call/cancel";
    case 11:
        return "/api/call/hangup";
    case 12:
        return "/upload_media_init";
    case 13:
        return "/upload_media_chunk";
    case 14:
        return "/upload_media_complete";
    case 15:
        return "/upload_media";
    case 16:
        return "/api/moments/publish";
    case 17:
        return "/api/moments/list";
    case 18:
        return "/api/moments/detail";
    case 19:
        return "/api/moments/delete";
    case 20:
        return "/api/moments/like";
    case 21:
        return "/api/moments/comment";
    case 22:
        return "/api/moments/comment/list";
    case 23:
        return "/api/moments/comment/like";
    default:
        return "";
    }
}

int RouteNotFoundStatus()
{
    return 404;
}

const char* RouteNotFoundBody()
{
    return R"({"error":404,"message":"route not found"})";
}

const char* JsonContentType()
{
    return "application/json";
}
} // namespace memochat::gate::h3::legacy::modules

#include "WinCompat.h"
#include "Http2Routes.h"
#include "GateRouteProfileRegistrar.h"
#include "adapters/h2/H2RouteAdapter.h"
#include "Http2AuthHandlers.h"
#include "Http2Handlers.h"
#include "modules/health/HealthRouteModule.h"
#include "routing/RouteRegistry.h"
#include <iostream>
#include <mutex>

namespace
{

struct RouteKey
{
    std::string method;
    std::string path;
    bool operator==(const RouteKey& other) const
    {
        return method == other.method && path == other.path;
    }
};

struct RouteKeyHash
{
    size_t operator()(const RouteKey& k) const
    {
        size_t h1 = std::hash<std::string>{}(k.method);
        size_t h2 = std::hash<std::string>{}(k.path);
        return h1 ^ (h2 << 1);
    }
};

static std::unordered_map<RouteKey, Http2Handler, RouteKeyHash> g_routes;
static std::mutex g_routes_mutex;

const memochat::gate::routing::RouteRegistry& SharedRouteRegistry()
{
    static const memochat::gate::routing::RouteRegistry registry = []
    {
        memochat::gate::routing::RouteRegistry routes;
        memochat::gate::modules::health::HealthRouteModule("GateServerHttp2").RegisterRoutes(routes);
        memochat::gate::profiles::RegisterAccount(routes);
        memochat::gate::profiles::RegisterAccountUserInfo(routes);
        memochat::gate::profiles::RegisterCall(routes);
        memochat::gate::profiles::RegisterMedia(routes);
        memochat::gate::profiles::RegisterMoments(routes);
        return routes;
    }();
    return registry;
}

void DispatchSharedRoute(const Http2Request& req, Http2Response& resp)
{
    if (!memochat::gate::adapters::h2::H2RouteAdapter::Dispatch(req, resp, SharedRouteRegistry()))
    {
        resp.SetStatus(404, "Not Found");
        resp.SetJsonBody(R"({"error":404,"message":"route not found"})");
    }
}

} // namespace

void Http2Routes::RegisterHandler(const std::string& method, const std::string& path, Http2Handler handler)
{
    std::lock_guard<std::mutex> lock(g_routes_mutex);
    g_routes[{method, path}] = std::move(handler);
}

void Http2Routes::HandleRequest(const Http2Request& req, Http2Response& resp)
{
    RouteKey key{req.method, req.path};
    std::lock_guard<std::mutex> lock(g_routes_mutex);
    auto it = g_routes.find(key);
    if (it != g_routes.end())
    {
        it->second(req, resp);
    }
    else
    {
        resp.SetStatus(404, "Not Found");
        resp.SetJsonBody(R"({"error":404,"message":"route not found"})");
    }
}

void Http2Routes::RegisterRoutes()
{
    // Health check endpoints
    RegisterHandler("GET", "/healthz", DispatchSharedRoute);
    RegisterHandler("GET", "/readyz", DispatchSharedRoute);

    // Auth routes
    RegisterHandler("POST", "/get_varifycode", Http2AuthHandlers::HandleGetVarifyCode);
    RegisterHandler("POST", "/user_register", Http2AuthHandlers::HandleUserRegister);
    RegisterHandler("POST", "/reset_pwd", Http2AuthHandlers::HandleResetPwd);
    RegisterHandler("POST", "/user_login", Http2AuthHandlers::HandleUserLogin);
    RegisterHandler("POST", "/user_logout", Http2AuthHandlers::HandleUserLogout);

    // Profile routes
    RegisterHandler("POST", "/user_update_profile", DispatchSharedRoute);
    RegisterHandler("POST", "/get_user_info", DispatchSharedRoute);

    // Call routes
    RegisterHandler("POST", "/api/call/start", DispatchSharedRoute);
    RegisterHandler("POST", "/api/call/accept", DispatchSharedRoute);
    RegisterHandler("POST", "/api/call/reject", DispatchSharedRoute);
    RegisterHandler("POST", "/api/call/cancel", DispatchSharedRoute);
    RegisterHandler("POST", "/api/call/hangup", DispatchSharedRoute);
    RegisterHandler("GET", "/api/call/token", DispatchSharedRoute);
    RegisterHandler("POST", "/api/call/token", DispatchSharedRoute);

    // Media routes
    RegisterHandler("POST", "/upload_media_init", DispatchSharedRoute);
    RegisterHandler("POST", "/upload_media_chunk", DispatchSharedRoute);
    RegisterHandler("GET", "/upload_media_status", DispatchSharedRoute);
    RegisterHandler("POST", "/upload_media_complete", DispatchSharedRoute);
    RegisterHandler("POST", "/upload_media", DispatchSharedRoute);
    RegisterHandler("GET", "/media/download", DispatchSharedRoute);

    // Moments routes
    RegisterHandler("POST", "/api/moments/publish", DispatchSharedRoute);
    RegisterHandler("POST", "/api/moments/list", DispatchSharedRoute);
    RegisterHandler("POST", "/api/moments/detail", DispatchSharedRoute);
    RegisterHandler("POST", "/api/moments/delete", DispatchSharedRoute);
    RegisterHandler("POST", "/api/moments/like", DispatchSharedRoute);
    RegisterHandler("POST", "/api/moments/comment", DispatchSharedRoute);
    RegisterHandler("POST", "/api/moments/comment/list", DispatchSharedRoute);
    RegisterHandler("POST", "/api/moments/comment/like", DispatchSharedRoute);

    std::cerr << "GateServerHttp2: all routes registered" << std::endl;
}

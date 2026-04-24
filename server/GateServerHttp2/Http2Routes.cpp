#include "WinCompat.h"
#include "Http2Routes.h"
#include "Http2AuthHandlers.h"
#include "Http2CallHandlers.h"
#include "Http2MediaHandlers.h"
#include "Http2ProfileHandlers.h"
#include "Http2Handlers.h"
#include "Http2MomentsSupport.h"
#include <iostream>
#include <mutex>

namespace {

struct RouteKey {
    std::string method;
    std::string path;
    bool operator==(const RouteKey& other) const {
        return method == other.method && path == other.path;
    }
};

struct RouteKeyHash {
    size_t operator()(const RouteKey& k) const {
        size_t h1 = std::hash<std::string>{}(k.method);
        size_t h2 = std::hash<std::string>{}(k.path);
        return h1 ^ (h2 << 1);
    }
};

static std::unordered_map<RouteKey, Http2Handler, RouteKeyHash> g_routes;
static std::mutex g_routes_mutex;

}  // namespace

void Http2Routes::RegisterHandler(const std::string& method, const std::string& path, Http2Handler handler) {
    std::lock_guard<std::mutex> lock(g_routes_mutex);
    g_routes[{method, path}] = std::move(handler);
}

void Http2Routes::HandleRequest(const Http2Request& req, Http2Response& resp) {
    RouteKey key{req.method, req.path};
    std::lock_guard<std::mutex> lock(g_routes_mutex);
    auto it = g_routes.find(key);
    if (it != g_routes.end()) {
        it->second(req, resp);
    } else {
        resp.SetStatus(404, "Not Found");
        resp.SetJsonBody(R"({"error":404,"message":"route not found"})");
    }
}

void Http2Routes::RegisterRoutes() {
    // Health check endpoints
    RegisterHandler("GET", "/healthz", [](const Http2Request& req, Http2Response& resp) {
        (void)req;
        resp.SetJsonBody(R"({"status":"ok","service":"GateServerHttp2"})");
    });
    RegisterHandler("GET", "/readyz", [](const Http2Request& req, Http2Response& resp) {
        (void)req;
        resp.SetJsonBody(R"({"status":"ready","service":"GateServerHttp2"})");
    });

    // Auth routes
    RegisterHandler("POST", "/get_varifycode", Http2AuthHandlers::HandleGetVarifyCode);
    RegisterHandler("POST", "/user_register", Http2AuthHandlers::HandleUserRegister);
    RegisterHandler("POST", "/reset_pwd", Http2AuthHandlers::HandleResetPwd);
    RegisterHandler("POST", "/user_login", Http2AuthHandlers::HandleUserLogin);
    RegisterHandler("POST", "/user_logout", Http2AuthHandlers::HandleUserLogout);

    // Profile routes
    RegisterHandler("POST", "/user_update_profile", Http2ProfileHandlers::HandleUserUpdateProfile);
    RegisterHandler("POST", "/get_user_info", Http2ProfileHandlers::HandleGetUserInfo);

    // Call routes
    RegisterHandler("POST", "/api/call/start", Http2CallHandlers::HandleCallStart);
    RegisterHandler("POST", "/api/call/accept", Http2CallHandlers::HandleCallAccept);
    RegisterHandler("POST", "/api/call/reject", Http2CallHandlers::HandleCallReject);
    RegisterHandler("POST", "/api/call/cancel", Http2CallHandlers::HandleCallCancel);
    RegisterHandler("POST", "/api/call/hangup", Http2CallHandlers::HandleCallHangup);
    RegisterHandler("GET", "/api/call/token", Http2CallHandlers::HandleCallToken);
    RegisterHandler("POST", "/api/call/token", Http2CallHandlers::HandleCallTokenPost);

    // Media routes
    RegisterHandler("POST", "/upload_media_init", Http2MediaHandlers::HandleUploadMediaInit);
    RegisterHandler("POST", "/upload_media_chunk", Http2MediaHandlers::HandleUploadMediaChunk);
    RegisterHandler("GET", "/upload_media_status", Http2MediaHandlers::HandleUploadMediaStatus);
    RegisterHandler("POST", "/upload_media_complete", Http2MediaHandlers::HandleUploadMediaComplete);
    RegisterHandler("POST", "/upload_media", Http2MediaHandlers::HandleUploadMedia);
    RegisterHandler("GET", "/media/download", Http2MediaHandlers::HandleMediaDownload);

    // Moments routes
    RegisterHttp2MomentsRoutes();

    std::cerr << "GateServerHttp2: all routes registered" << std::endl;
}

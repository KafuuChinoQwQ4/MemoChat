#include "GateHttp3ServiceRoutes.h"

#include "GateHttp3Connection.h"
#include "GateRouteProfileRegistrar.h"
#include "LogicSystem.h"
#include "adapters/h3/H3RouteAdapter.h"
#include "modules/health/HealthRouteModule.h"
#include "routing/RouteRegistry.h"

#include <memory>

namespace GateHttp3ServiceImpl
{

namespace
{

memochat::gate::routing::RouteRegistry& SharedRouteRegistry()
{
    static memochat::gate::routing::RouteRegistry registry;
    return registry;
}

void EnsureSharedRouteRegistry()
{
    static const bool registered = []
    {
        auto& registry = SharedRouteRegistry();
        memochat::gate::modules::health::HealthRouteModule("GateServer-HTTP3").RegisterRoutes(registry);
        memochat::gate::profiles::RegisterRegister(registry);
        memochat::gate::profiles::RegisterLogin(registry);
        memochat::gate::profiles::RegisterAccount(registry);
        memochat::gate::profiles::RegisterAccountUserInfo(registry);
        memochat::gate::profiles::RegisterCall(registry);
        memochat::gate::profiles::RegisterMedia(registry);
        memochat::gate::profiles::RegisterMoments(registry);
        return true;
    }();
    (void) registered;
}

bool DispatchSharedRoute(const std::shared_ptr<GateHttp3Connection>& connection)
{
    EnsureSharedRouteRegistry();
    if (memochat::gate::adapters::h3::H3RouteAdapter::Dispatch(connection, SharedRouteRegistry()))
    {
        return true;
    }

    if (!connection)
    {
        return false;
    }

    connection->SendResponse(404, R"({"error":404,"message":"route not found"})", "application/json");
    return true;
}

} // namespace

void RegisterRoutes(LogicSystem& logic)
{
    logic.RegGet("/healthz", DispatchSharedRoute);
    logic.RegGet("/readyz", DispatchSharedRoute);

    logic.RegPost("/get_varifycode", DispatchSharedRoute);
    logic.RegPost("/user_register", DispatchSharedRoute);
    logic.RegPost("/reset_pwd", DispatchSharedRoute);
    logic.RegPost("/user_login", DispatchSharedRoute);

    logic.RegPost("/user_update_profile", DispatchSharedRoute);
    logic.RegPost("/get_user_info", DispatchSharedRoute);

    logic.RegPost("/api/call/token", DispatchSharedRoute);
    logic.RegPost("/api/call/start", DispatchSharedRoute);
    logic.RegPost("/api/call/accept", DispatchSharedRoute);
    logic.RegPost("/api/call/reject", DispatchSharedRoute);
    logic.RegPost("/api/call/cancel", DispatchSharedRoute);
    logic.RegPost("/api/call/hangup", DispatchSharedRoute);

    logic.RegPost("/upload_media_init", DispatchSharedRoute);
    logic.RegPost("/upload_media_chunk", DispatchSharedRoute);
    logic.RegPost("/upload_media_complete", DispatchSharedRoute);
    logic.RegPost("/upload_media", DispatchSharedRoute);
    logic.RegGet("/upload_media_status", DispatchSharedRoute);
    logic.RegGet("/media/download", DispatchSharedRoute);

    logic.RegPost("/api/moments/publish", DispatchSharedRoute);
    logic.RegPost("/api/moments/list", DispatchSharedRoute);
    logic.RegPost("/api/moments/detail", DispatchSharedRoute);
    logic.RegPost("/api/moments/delete", DispatchSharedRoute);
    logic.RegPost("/api/moments/like", DispatchSharedRoute);
    logic.RegPost("/api/moments/comment", DispatchSharedRoute);
    logic.RegPost("/api/moments/comment/list", DispatchSharedRoute);
    logic.RegPost("/api/moments/comment/like", DispatchSharedRoute);
}

} // namespace GateHttp3ServiceImpl

void GateHttp3Service::RegisterRoutes(LogicSystem& logic)
{
    GateHttp3ServiceImpl::RegisterRoutes(logic);
}

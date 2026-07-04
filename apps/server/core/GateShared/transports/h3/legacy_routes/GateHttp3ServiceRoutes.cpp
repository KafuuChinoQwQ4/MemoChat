#include "GateHttp3ServiceRoutes.hpp"

#include "GateHttp3Connection.hpp"
#include "GateRouteProfileRegistrar.hpp"
#include "LogicSystem.hpp"
#include "adapters/h3/H3RouteAdapter.hpp"
#include "modules/health/HealthRouteModule.hpp"
#include "routing/RouteRegistry.hpp"

#include <memory>

import memochat.gate.h3_legacy_route_algorithms;

namespace GateHttp3ServiceImpl
{

namespace
{
namespace h3_legacy_modules = memochat::gate::h3::legacy::modules;

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

    connection->SendResponse(h3_legacy_modules::RouteNotFoundStatus(),
                             h3_legacy_modules::RouteNotFoundBody(),
                             h3_legacy_modules::JsonContentType());
    return true;
}

} // namespace

void RegisterRoutes(LogicSystem& logic)
{
    for (int index = 0; index < h3_legacy_modules::GetRouteCount(); ++index)
    {
        logic.RegGet(h3_legacy_modules::GetRoutePathAt(index), DispatchSharedRoute);
    }

    for (int index = 0; index < h3_legacy_modules::PostRouteCount(); ++index)
    {
        logic.RegPost(h3_legacy_modules::PostRoutePathAt(index), DispatchSharedRoute);
    }
}

} // namespace GateHttp3ServiceImpl

void GateHttp3Service::RegisterRoutes(LogicSystem& logic)
{
    GateHttp3ServiceImpl::RegisterRoutes(logic);
}

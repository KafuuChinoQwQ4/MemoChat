#include "GateRouteProfileRegistrar.hpp"

#include "modules/call/CallRouteModule.hpp"

namespace memochat::gate::profiles
{

void RegisterCall(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::call::CallRouteModule().RegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

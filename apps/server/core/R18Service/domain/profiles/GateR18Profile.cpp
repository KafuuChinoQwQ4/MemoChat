#include "GateRouteProfileRegistrar.h"

#include "modules/r18/R18RouteModule.h"

namespace memochat::gate::profiles
{

void RegisterR18(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::r18::R18RouteModule().RegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

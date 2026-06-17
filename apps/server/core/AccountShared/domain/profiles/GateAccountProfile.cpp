#include "GateRouteProfileRegistrar.h"

#include "modules/profile/ProfileRouteModule.h"

namespace memochat::gate::profiles
{

void RegisterAccount(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::profile::ProfileRouteModule().RegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

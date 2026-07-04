#include "GateRouteProfileRegistrar.hpp"

#include "modules/profile/ProfileRouteModule.hpp"

namespace memochat::gate::profiles
{

void RegisterAccount(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::profile::ProfileRouteModule().RegisterRoutes(registry);
}

void RegisterAccountUserInfo(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::profile::ProfileRouteModule::RegisterUserInfoRoutes(registry);
}

} // namespace memochat::gate::profiles

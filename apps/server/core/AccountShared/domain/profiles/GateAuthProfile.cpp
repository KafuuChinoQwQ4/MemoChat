#include "GateRouteProfileRegistrar.hpp"

#include "modules/auth/AuthRouteModule.hpp"

namespace memochat::gate::profiles
{

void RegisterLogin(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::auth::AuthRouteModule::RegisterLoginRoutes(registry);
}

void RegisterRegister(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::auth::AuthRouteModule::RegisterRegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

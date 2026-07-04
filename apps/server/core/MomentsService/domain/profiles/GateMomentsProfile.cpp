#include "GateRouteProfileRegistrar.hpp"

#include "modules/moments/MomentsRouteModule.hpp"

namespace memochat::gate::profiles
{

void RegisterMoments(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::moments::MomentsRouteModule().RegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

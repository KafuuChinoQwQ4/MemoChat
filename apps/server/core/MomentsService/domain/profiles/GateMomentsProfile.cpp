#include "GateRouteProfileRegistrar.h"

#include "modules/moments/MomentsRouteModule.h"

namespace memochat::gate::profiles
{

void RegisterMoments(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::moments::MomentsRouteModule().RegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

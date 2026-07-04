#include "GateRouteProfileRegistrar.hpp"

#include "modules/media/MediaRouteModule.hpp"

namespace memochat::gate::profiles
{

void RegisterMedia(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::media::MediaRouteModule().RegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

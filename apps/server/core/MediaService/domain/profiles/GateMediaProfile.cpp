#include "GateRouteProfileRegistrar.h"

#include "modules/media/MediaRouteModule.h"

namespace memochat::gate::profiles
{

void RegisterMedia(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::media::MediaRouteModule().RegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

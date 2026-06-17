#include "GateRouteProfileRegistrar.h"

#include "modules/ai/AIRouteModule.h"

namespace memochat::gate::profiles
{

void RegisterAIGateway(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::ai::AIRouteModule().RegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

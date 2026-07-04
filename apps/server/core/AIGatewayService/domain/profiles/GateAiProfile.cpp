#include "GateRouteProfileRegistrar.hpp"

#include "modules/ai/AIRouteModule.hpp"

namespace memochat::gate::profiles
{

void RegisterAIGateway(memochat::gate::routing::RouteRegistry& registry)
{
    memochat::gate::modules::ai::AIRouteModule().RegisterRoutes(registry);
}

} // namespace memochat::gate::profiles

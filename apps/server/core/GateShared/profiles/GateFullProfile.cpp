#include "GateRouteProfileRegistrar.h"

namespace memochat::gate::profiles
{

void RegisterFull(memochat::gate::routing::RouteRegistry& registry)
{
    RegisterAIGateway(registry);
    RegisterMedia(registry);
    RegisterMoments(registry);
    RegisterCall(registry);
    RegisterR18(registry);
    RegisterRegister(registry);
    RegisterLogin(registry);
    RegisterAccount(registry);
}

} // namespace memochat::gate::profiles

#pragma once

#include "routing/RouteRegistry.h"

namespace memochat::gate::profiles
{

void RegisterAIGateway(memochat::gate::routing::RouteRegistry& registry);
void RegisterMedia(memochat::gate::routing::RouteRegistry& registry);
void RegisterMoments(memochat::gate::routing::RouteRegistry& registry);
void RegisterCall(memochat::gate::routing::RouteRegistry& registry);
void RegisterR18(memochat::gate::routing::RouteRegistry& registry);
void RegisterAccount(memochat::gate::routing::RouteRegistry& registry);
void RegisterAccountUserInfo(memochat::gate::routing::RouteRegistry& registry);
void RegisterLogin(memochat::gate::routing::RouteRegistry& registry);
void RegisterRegister(memochat::gate::routing::RouteRegistry& registry);
void RegisterFull(memochat::gate::routing::RouteRegistry& registry);

} // namespace memochat::gate::profiles

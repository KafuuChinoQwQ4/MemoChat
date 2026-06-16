#pragma once

#include "routing/RouteModule.h"

namespace memochat::gate::modules::profile
{

class ProfileRouteModule final : public memochat::gate::routing::RouteModule
{
public:
    void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override;
};

} // namespace memochat::gate::modules::profile

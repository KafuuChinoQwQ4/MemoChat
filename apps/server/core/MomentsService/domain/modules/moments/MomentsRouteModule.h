#pragma once

#include "routing/RouteModule.h"

namespace memochat::gate::modules::moments
{

class MomentsRouteModule final : public memochat::gate::routing::RouteModule
{
public:
    void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override;
};

} // namespace memochat::gate::modules::moments

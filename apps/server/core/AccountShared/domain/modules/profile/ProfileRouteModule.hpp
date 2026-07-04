#pragma once

#include "routing/RouteModule.hpp"
#include "routing/RouteSchema.hpp"

#include <vector>

namespace memochat::gate::modules::profile
{

class ProfileRouteModule final : public memochat::gate::routing::RouteModule
{
public:
    void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override;
    static void RegisterUserInfoRoutes(memochat::gate::routing::RouteRegistry& registry);

    static std::vector<memochat::gate::routing::RouteSchemaDescriptor> RouteSchemas();
};

} // namespace memochat::gate::modules::profile

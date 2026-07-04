#pragma once

#include "routing/RouteModule.hpp"
#include "routing/RouteSchema.hpp"

#include <vector>

namespace memochat::gate::modules::moments
{

class MomentsRouteModule final : public memochat::gate::routing::RouteModule
{
public:
    void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override;

    static std::vector<memochat::gate::routing::RouteSchemaDescriptor> RouteSchemas();
};

} // namespace memochat::gate::modules::moments

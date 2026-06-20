#pragma once

#include "routing/RouteModule.h"
#include "routing/RouteSchema.h"

#include <vector>

namespace memochat::gate::modules::media
{

class MediaRouteModule final : public memochat::gate::routing::RouteModule
{
public:
    void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override;

    static std::vector<memochat::gate::routing::RouteSchemaDescriptor> RouteSchemas();
};

} // namespace memochat::gate::modules::media

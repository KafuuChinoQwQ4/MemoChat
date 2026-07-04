#pragma once

#include "routing/RouteModule.hpp"
#include "routing/RouteSchema.hpp"

#include <vector>

namespace memochat::gate::modules::r18
{

class R18RouteModule final : public memochat::gate::routing::RouteModule
{
public:
    void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override;

    static std::vector<memochat::gate::routing::RouteSchemaDescriptor> RouteSchemas();
};

} // namespace memochat::gate::modules::r18

#pragma once

#include "routing/RouteModule.h"

#include <string>

namespace memochat::gate::modules::health
{

class HealthRouteModule final : public memochat::gate::routing::RouteModule
{
public:
    explicit HealthRouteModule(std::string service_name = "GateServer");
    void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override;

private:
    std::string service_name_;
};

} // namespace memochat::gate::modules::health

#pragma once

#include "routing/RouteModule.hpp"

#include <functional>
#include <string>

namespace memochat::gate::modules::health
{

class HealthRouteModule final : public memochat::gate::routing::RouteModule
{
public:
    using ReadinessCheck = std::function<bool(std::string* error)>;

    explicit HealthRouteModule(std::string service_name = "GateServer");
    // RegisterRoutes captures a thread-safe snapshot; set the process probe
    // before the route profile is constructed.
    static void SetReadinessCheck(ReadinessCheck check);
    void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override;

private:
    std::string service_name_;
};

} // namespace memochat::gate::modules::health

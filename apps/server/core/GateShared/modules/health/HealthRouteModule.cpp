#include "modules/health/HealthRouteModule.hpp"

#include "routing/RouteRegistry.hpp"

#include "json/GlazeCompat.hpp"

#include <utility>

import memochat.gate.health_route_algorithms;

namespace memochat::gate::modules::health
{
namespace health_modules = memochat::gate::health::modules;

HealthRouteModule::HealthRouteModule(std::string service_name)
    : service_name_(std::move(service_name))
{
}

void HealthRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register("GET",
                      health_modules::HealthPath(),
                      [service_name = service_name_](const memochat::gate::routing::GateRequest&,
                                                     memochat::gate::routing::GateResponse& response)
                      {
                          memochat::json::JsonValue root;
                          root["status"] = health_modules::HealthStatusText();
                          root["service"] = service_name;
                          response.status = health_modules::SuccessfulProbeStatus();
                          response.content_type = health_modules::JsonContentType();
                          response.body = root.toStyledString();
                          return true;
                      });

    registry.Register("GET",
                      health_modules::ReadinessPath(),
                      [service_name = service_name_](const memochat::gate::routing::GateRequest&,
                                                     memochat::gate::routing::GateResponse& response)
                      {
                          memochat::json::JsonValue root;
                          root["status"] = health_modules::ReadinessStatusText();
                          root["service"] = service_name;
                          response.status = health_modules::SuccessfulProbeStatus();
                          response.content_type = health_modules::JsonContentType();
                          response.body = root.toStyledString();
                          return true;
                      });
}

} // namespace memochat::gate::modules::health

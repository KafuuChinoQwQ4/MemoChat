#include "modules/health/HealthRouteModule.h"

#include "routing/RouteRegistry.h"

#include "json/GlazeCompat.h"

#include <utility>

namespace memochat::gate::modules::health
{

HealthRouteModule::HealthRouteModule(std::string service_name)
    : service_name_(std::move(service_name))
{
}

void HealthRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register("GET",
                      "/healthz",
                      [service_name = service_name_](const memochat::gate::routing::GateRequest&,
                                                     memochat::gate::routing::GateResponse& response)
                      {
                          memochat::json::JsonValue root;
                          root["status"] = "ok";
                          root["service"] = service_name;
                          response.status = 200;
                          response.content_type = "application/json";
                          response.body = root.toStyledString();
                          return true;
                      });

    registry.Register("GET",
                      "/readyz",
                      [service_name = service_name_](const memochat::gate::routing::GateRequest&,
                                                     memochat::gate::routing::GateResponse& response)
                      {
                          memochat::json::JsonValue root;
                          root["status"] = "ready";
                          root["service"] = service_name;
                          response.status = 200;
                          response.content_type = "application/json";
                          response.body = root.toStyledString();
                          return true;
                      });
}

} // namespace memochat::gate::modules::health

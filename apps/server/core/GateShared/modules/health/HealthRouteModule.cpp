#include "modules/health/HealthRouteModule.hpp"

#include "routing/RouteRegistry.hpp"

#include "json/GlazeCompat.hpp"

#include <mutex>
#include <utility>

import memochat.gate.health_route_algorithms;

namespace memochat::gate::modules::health
{
namespace health_modules = memochat::gate::health::modules;

namespace
{
struct ReadinessCheckState
{
    std::mutex mutex;
    HealthRouteModule::ReadinessCheck check;
};

ReadinessCheckState& ReadinessCheckStorage()
{
    static ReadinessCheckState state;
    return state;
}
} // namespace

HealthRouteModule::HealthRouteModule(std::string service_name)
    : service_name_(std::move(service_name))
{
}

void HealthRouteModule::SetReadinessCheck(ReadinessCheck check)
{
    auto& state = ReadinessCheckStorage();
    std::lock_guard<std::mutex> lock(state.mutex);
    state.check = std::move(check);
}

void HealthRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    ReadinessCheck readiness_check;
    {
        auto& state = ReadinessCheckStorage();
        std::lock_guard<std::mutex> lock(state.mutex);
        readiness_check = state.check;
    }
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
                      [service_name = service_name_, readiness_check](const memochat::gate::routing::GateRequest&,
                                                                      memochat::gate::routing::GateResponse& response)
                      {
                          memochat::json::JsonValue root;
                          root["status"] = health_modules::ReadinessStatusText();
                          root["service"] = service_name;
                          std::string readiness_error;
                          bool ready = false;
                          if (!readiness_check)
                          {
                              readiness_error = "readiness check is not configured";
                          }
                          else
                          {
                              ready = readiness_check(&readiness_error);
                          }
                          if (!ready)
                          {
                              root["status"] = health_modules::NotReadyStatusText();
                              if (!readiness_error.empty())
                              {
                                  root["error"] = readiness_error;
                              }
                          }
                          response.status = ready ? health_modules::SuccessfulProbeStatus()
                                                  : health_modules::ReadinessFailureStatus();
                          response.content_type = health_modules::JsonContentType();
                          response.body = root.toStyledString();
                          return true;
                      });
}

} // namespace memochat::gate::modules::health

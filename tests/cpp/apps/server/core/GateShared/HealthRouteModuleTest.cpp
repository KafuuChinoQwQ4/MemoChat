#include "modules/health/HealthRouteModule.hpp"
#include "routing/RouteRegistry.hpp"

#include "json/GlazeCompat.hpp"

#include <gtest/gtest.h>

namespace memochat::tests::gate::health
{
const char* HealthPath();
const char* ReadinessPath();
const char* HealthStatusText();
const char* ReadinessStatusText();
const char* JsonContentType();
int SuccessfulProbeStatus();
} // namespace memochat::tests::gate::health

namespace
{
using memochat::gate::modules::health::HealthRouteModule;
using memochat::gate::routing::GateRequest;
using memochat::gate::routing::GateResponse;
using memochat::gate::routing::RouteRegistry;

memochat::json::JsonValue ParseBody(const std::string& body)
{
    memochat::json::JsonValue root;
    memochat::json::JsonReader reader;
    EXPECT_TRUE(reader.parse(body, root)) << body;
    return root;
}
} // namespace

TEST(HealthRouteModuleTest, RegistersHealthAndReadinessRoutesThroughImportedAlgorithms)
{
    RouteRegistry registry;
    HealthRouteModule("MemoChat-Gate").RegisterRoutes(registry);

    GateResponse health_response;
    ASSERT_TRUE(registry.Dispatch(GateRequest{.method = "get", .path = memochat::tests::gate::health::HealthPath()},
                                  health_response));
    EXPECT_EQ(health_response.status, memochat::tests::gate::health::SuccessfulProbeStatus());
    EXPECT_EQ(health_response.content_type, memochat::tests::gate::health::JsonContentType());
    const auto health = ParseBody(health_response.body);
    EXPECT_EQ(health["status"].asString(), memochat::tests::gate::health::HealthStatusText());
    EXPECT_EQ(health["service"].asString(), "MemoChat-Gate");

    GateResponse readiness_response;
    ASSERT_TRUE(registry.Dispatch(GateRequest{.method = "GET", .path = memochat::tests::gate::health::ReadinessPath()},
                                  readiness_response));
    EXPECT_EQ(readiness_response.status, memochat::tests::gate::health::SuccessfulProbeStatus());
    EXPECT_EQ(readiness_response.content_type, memochat::tests::gate::health::JsonContentType());
    const auto readiness = ParseBody(readiness_response.body);
    EXPECT_EQ(readiness["status"].asString(), memochat::tests::gate::health::ReadinessStatusText());
    EXPECT_EQ(readiness["service"].asString(), "MemoChat-Gate");
}

TEST(HealthRouteModuleTest, DoesNotRegisterNonProbePaths)
{
    RouteRegistry registry;
    HealthRouteModule().RegisterRoutes(registry);

    GateResponse response;
    EXPECT_FALSE(registry.Dispatch(GateRequest{.method = "GET", .path = "/healthz/extra"}, response));
    EXPECT_FALSE(registry.Dispatch(GateRequest{.method = "GET", .path = "/readyz?full=1"}, response));
}

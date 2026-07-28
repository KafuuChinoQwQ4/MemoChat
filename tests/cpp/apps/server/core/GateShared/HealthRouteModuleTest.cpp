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

TEST(HealthRouteModuleTest, RegistersHealthAndFailsClosedWithoutReadinessCheck)
{
    HealthRouteModule::SetReadinessCheck({});
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
    EXPECT_EQ(readiness_response.status, 503);
    EXPECT_EQ(readiness_response.content_type, memochat::tests::gate::health::JsonContentType());
    const auto readiness = ParseBody(readiness_response.body);
    EXPECT_EQ(readiness["status"].asString(), "not_ready");
    EXPECT_EQ(readiness["service"].asString(), "MemoChat-Gate");
    EXPECT_EQ(readiness["error"].asString(), "readiness check is not configured");
}

TEST(HealthRouteModuleTest, DoesNotRegisterNonProbePaths)
{
    RouteRegistry registry;
    HealthRouteModule().RegisterRoutes(registry);

    GateResponse response;
    EXPECT_FALSE(registry.Dispatch(GateRequest{.method = "GET", .path = "/healthz/extra"}, response));
    EXPECT_FALSE(registry.Dispatch(GateRequest{.method = "GET", .path = "/readyz?full=1"}, response));
}

TEST(HealthRouteModuleTest, ReadinessReturnsSuccessWhenConfiguredProbeSucceeds)
{
    HealthRouteModule::SetReadinessCheck(
        [](std::string*)
        {
            return true;
        });

    RouteRegistry registry;
    HealthRouteModule("MemoChat-Gate").RegisterRoutes(registry);
    GateResponse response;
    ASSERT_TRUE(registry.Dispatch(GateRequest{.method = "GET", .path = "/readyz"}, response));
    EXPECT_EQ(response.status, memochat::tests::gate::health::SuccessfulProbeStatus());
    const auto body = ParseBody(response.body);
    EXPECT_EQ(body["status"].asString(), memochat::tests::gate::health::ReadinessStatusText());
    EXPECT_FALSE(body.isMember("error"));
    HealthRouteModule::SetReadinessCheck({});
}

TEST(HealthRouteModuleTest, ReadinessReturnsServiceUnavailableWhenProbeFails)
{
    HealthRouteModule::SetReadinessCheck(
        [](std::string* error)
        {
            if (error != nullptr)
            {
                *error = "dependency unavailable";
            }
            return false;
        });

    RouteRegistry registry;
    HealthRouteModule("MemoChat-Gate").RegisterRoutes(registry);
    GateResponse response;
    ASSERT_TRUE(registry.Dispatch(GateRequest{.method = "GET", .path = "/readyz"}, response));
    EXPECT_EQ(response.status, 503);
    const auto body = ParseBody(response.body);
    EXPECT_EQ(body["status"].asString(), "not_ready");
    EXPECT_EQ(body["error"].asString(), "dependency unavailable");
    HealthRouteModule::SetReadinessCheck({});
}

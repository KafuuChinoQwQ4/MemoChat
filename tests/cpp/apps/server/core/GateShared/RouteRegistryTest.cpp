#include "routing/RouteRegistry.hpp"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace
{
using memochat::gate::routing::GateRequest;
using memochat::gate::routing::GateResponse;
using memochat::gate::routing::RouteRegistry;

RouteRegistry::RouteHandler HandlerReturning(std::string body)
{
    return [body = std::move(body)](const GateRequest&, GateResponse& response)
    {
        response.body = body;
        return true;
    };
}
} // namespace

TEST(RouteRegistryTest, DispatchesExactRoutesWithNormalizedMethod)
{
    RouteRegistry registry;
    registry.Register("POST", "/user_login", HandlerReturning("login"));

    GateResponse response;
    const bool handled = registry.Dispatch(GateRequest{.method = "post", .path = "/user_login"}, response);

    EXPECT_TRUE(handled);
    EXPECT_EQ(response.body, "login");
}

TEST(RouteRegistryTest, DispatchesPrefixRoutesOnlyAtPathBoundaries)
{
    RouteRegistry registry;
    registry.RegisterPrefix("POST", "/api/moments", HandlerReturning("moments"));

    const std::vector<std::pair<std::string, bool>> cases = {
        {"/api/moments", true},
        {"/api/moments/feed", true},
        {"/api/moments?cursor=1", true},
        {"/api/moments_extra", false},
        {"/api/moment", false},
    };

    for (const auto& [path, expected] : cases)
    {
        GateResponse response;
        const bool handled = registry.Dispatch(GateRequest{.method = "post", .path = path}, response);
        EXPECT_EQ(handled, expected) << path;
        if (expected)
        {
            EXPECT_EQ(response.body, "moments") << path;
        }
    }
}

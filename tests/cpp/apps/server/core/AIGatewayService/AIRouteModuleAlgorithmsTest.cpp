#include <gtest/gtest.h>

#include <string_view>

namespace memochat::gate::modules::ai::testsupport
{
int NormalizeRouteTimeoutSeconds(int value);
bool RouteTargetStartsWith(std::string_view target, std::string_view prefix);
} // namespace memochat::gate::modules::ai::testsupport

TEST(AIRouteModuleAlgorithmsTest, NormalizesProxyTimeoutLowerBound)
{
    EXPECT_EQ(memochat::gate::modules::ai::testsupport::NormalizeRouteTimeoutSeconds(-5), 1);
    EXPECT_EQ(memochat::gate::modules::ai::testsupport::NormalizeRouteTimeoutSeconds(0), 1);
    EXPECT_EQ(memochat::gate::modules::ai::testsupport::NormalizeRouteTimeoutSeconds(1), 1);
    EXPECT_EQ(memochat::gate::modules::ai::testsupport::NormalizeRouteTimeoutSeconds(300), 300);
}

TEST(AIRouteModuleAlgorithmsTest, DetectsPrefixProxyTargets)
{
    using memochat::gate::modules::ai::testsupport::RouteTargetStartsWith;

    EXPECT_TRUE(RouteTargetStartsWith("/ai/pet/stream", "/ai/pet"));
    EXPECT_TRUE(RouteTargetStartsWith("/ai/games", "/ai/games"));
    EXPECT_TRUE(RouteTargetStartsWith("/ai/games/rooms?id=1", "/ai/games"));
    EXPECT_TRUE(RouteTargetStartsWith("/ai/pet", ""));
    EXPECT_FALSE(RouteTargetStartsWith("/api/pet", "/ai/pet"));
    EXPECT_FALSE(RouteTargetStartsWith("/ai", "/ai/pet"));
}

#include <gtest/gtest.h>

#include <cstring>

// Behavior pin for the TelemetryConfig primitive algorithms module. These tests
// lock the exact bool-token vocabulary, the inclusive [0,1] sample-ratio clamp,
// and the empty-protocol default that TelemetryConfig::FromGetter relies on.
// Signatures mirror the re-exposed helpers in TelemetryConfigAlgorithmsConsumer.cpp.
namespace memochat::tests::logging::telemetry_config
{
bool IsTrueBoolToken(const char* data, unsigned long long size);
bool IsFalseBoolToken(const char* data, unsigned long long size);
double ClampSampleRatio(double value);
bool ShouldUseDefaultProtocol(bool protocol_empty);
const char* DefaultProtocol();
} // namespace memochat::tests::logging::telemetry_config

namespace
{
using namespace memochat::tests::logging::telemetry_config;

bool TrueTok(const char* s)
{
    return IsTrueBoolToken(s, std::strlen(s));
}
bool FalseTok(const char* s)
{
    return IsFalseBoolToken(s, std::strlen(s));
}
} // namespace

TEST(TelemetryConfigAlgorithmsTest, TrueBoolTokensAreExactlyTheKnownSet)
{
    EXPECT_TRUE(TrueTok("1"));
    EXPECT_TRUE(TrueTok("true"));
    EXPECT_TRUE(TrueTok("yes"));
    EXPECT_TRUE(TrueTok("on"));
    EXPECT_FALSE(TrueTok("0"));
    EXPECT_FALSE(TrueTok(""));
    EXPECT_FALSE(TrueTok("on "));
    EXPECT_FALSE(TrueTok("ON")); // case handled by ToLower before the module call
}

TEST(TelemetryConfigAlgorithmsTest, FalseBoolTokensAreExactlyTheKnownSet)
{
    EXPECT_TRUE(FalseTok("0"));
    EXPECT_TRUE(FalseTok("false"));
    EXPECT_TRUE(FalseTok("no"));
    EXPECT_TRUE(FalseTok("off"));
    EXPECT_FALSE(FalseTok("1"));
    EXPECT_FALSE(FalseTok(""));
    EXPECT_FALSE(FalseTok("nope"));
}

TEST(TelemetryConfigAlgorithmsTest, SampleRatioClampsToUnitInterval)
{
    EXPECT_DOUBLE_EQ(ClampSampleRatio(-0.0001), 0.0);
    EXPECT_DOUBLE_EQ(ClampSampleRatio(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(ClampSampleRatio(1.5), 1.0);
    EXPECT_DOUBLE_EQ(ClampSampleRatio(2.0), 1.0);
    // In-range values pass through unchanged, including the exact bounds.
    EXPECT_DOUBLE_EQ(ClampSampleRatio(0.0), 0.0);
    EXPECT_DOUBLE_EQ(ClampSampleRatio(0.25), 0.25);
    EXPECT_DOUBLE_EQ(ClampSampleRatio(1.0), 1.0);
}

TEST(TelemetryConfigAlgorithmsTest, EmptyProtocolFallsBackToDefault)
{
    EXPECT_TRUE(ShouldUseDefaultProtocol(true));
    EXPECT_FALSE(ShouldUseDefaultProtocol(false));
    EXPECT_STREQ(DefaultProtocol(), "zipkin-json");
}

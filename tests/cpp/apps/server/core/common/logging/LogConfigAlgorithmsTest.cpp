#include <gtest/gtest.h>

#include <cstring>

// Behavior pin for the LogConfig primitive algorithms module. These tests lock
// the exact bool-token vocabulary, non-positive clamp defaults, and rotate/level
// validation fallbacks that LogConfig::FromGetter relies on. Signatures mirror
// the re-exposed helpers in LogConfigAlgorithmsConsumer.cpp.
namespace memochat::tests::logging::log_config
{
bool IsTrueBoolToken(const char* data, unsigned long long size);
bool IsFalseBoolToken(const char* data, unsigned long long size);
int NormalizeMaxFiles(int value);
int NormalizeMaxSizeMb(int value);
bool IsValidRotateMode(const char* data, unsigned long long size);
const char* DefaultRotateMode();
bool IsValidLogLevel(const char* data, unsigned long long size);
const char* DefaultLogLevel();
} // namespace memochat::tests::logging::log_config

namespace
{
using namespace memochat::tests::logging::log_config;

bool TrueTok(const char* s)
{
    return IsTrueBoolToken(s, std::strlen(s));
}
bool FalseTok(const char* s)
{
    return IsFalseBoolToken(s, std::strlen(s));
}
bool ValidRotate(const char* s)
{
    return IsValidRotateMode(s, std::strlen(s));
}
bool ValidLevel(const char* s)
{
    return IsValidLogLevel(s, std::strlen(s));
}
} // namespace

TEST(LogConfigAlgorithmsTest, TrueBoolTokensAreExactlyTheKnownSet)
{
    EXPECT_TRUE(TrueTok("1"));
    EXPECT_TRUE(TrueTok("true"));
    EXPECT_TRUE(TrueTok("yes"));
    EXPECT_TRUE(TrueTok("on"));
    // Non-members and near-misses are rejected.
    EXPECT_FALSE(TrueTok("0"));
    EXPECT_FALSE(TrueTok("false"));
    EXPECT_FALSE(TrueTok(""));
    EXPECT_FALSE(TrueTok("true1"));
    EXPECT_FALSE(TrueTok("tru"));
    EXPECT_FALSE(TrueTok("TRUE")); // case handled by ToLower before the module call
}

TEST(LogConfigAlgorithmsTest, FalseBoolTokensAreExactlyTheKnownSet)
{
    EXPECT_TRUE(FalseTok("0"));
    EXPECT_TRUE(FalseTok("false"));
    EXPECT_TRUE(FalseTok("no"));
    EXPECT_TRUE(FalseTok("off"));
    EXPECT_FALSE(FalseTok("1"));
    EXPECT_FALSE(FalseTok("true"));
    EXPECT_FALSE(FalseTok(""));
    EXPECT_FALSE(FalseTok("offf"));
    EXPECT_FALSE(FalseTok("of"));
}

TEST(LogConfigAlgorithmsTest, NonPositiveMaxFilesFallsBackToFourteen)
{
    EXPECT_EQ(NormalizeMaxFiles(0), 14);
    EXPECT_EQ(NormalizeMaxFiles(-1), 14);
    EXPECT_EQ(NormalizeMaxFiles(-100), 14);
    // Positive values pass through unchanged.
    EXPECT_EQ(NormalizeMaxFiles(1), 1);
    EXPECT_EQ(NormalizeMaxFiles(14), 14);
    EXPECT_EQ(NormalizeMaxFiles(30), 30);
}

TEST(LogConfigAlgorithmsTest, NonPositiveMaxSizeMbFallsBackToHundred)
{
    EXPECT_EQ(NormalizeMaxSizeMb(0), 100);
    EXPECT_EQ(NormalizeMaxSizeMb(-5), 100);
    EXPECT_EQ(NormalizeMaxSizeMb(1), 1);
    EXPECT_EQ(NormalizeMaxSizeMb(100), 100);
    EXPECT_EQ(NormalizeMaxSizeMb(512), 512);
}

TEST(LogConfigAlgorithmsTest, RotateModeValidationAndDefault)
{
    EXPECT_TRUE(ValidRotate("daily"));
    EXPECT_TRUE(ValidRotate("size"));
    EXPECT_FALSE(ValidRotate("hourly"));
    EXPECT_FALSE(ValidRotate(""));
    EXPECT_FALSE(ValidRotate("Daily")); // case handled by ToLower before the module call
    EXPECT_STREQ(DefaultRotateMode(), "daily");
}

TEST(LogConfigAlgorithmsTest, LogLevelValidationAndDefault)
{
    for (const char* level : {"trace", "debug", "info", "warn", "error", "critical"})
    {
        EXPECT_TRUE(ValidLevel(level)) << level;
    }
    EXPECT_FALSE(ValidLevel("verbose"));
    EXPECT_FALSE(ValidLevel("warning"));
    EXPECT_FALSE(ValidLevel(""));
    EXPECT_FALSE(ValidLevel("INFO")); // case handled by ToLower before the module call
    EXPECT_STREQ(DefaultLogLevel(), "info");
}

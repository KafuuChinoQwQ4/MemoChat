#include "logging/LogConfig.hpp"
#include "logging/TelemetryConfig.hpp"

#include <gtest/gtest.h>

#include <string>

namespace
{
std::string LoggingGetter(const std::string& section, const std::string& key)
{
    if (section == "Log" && key == "Level")
    {
        return "  WARN  ";
    }
    if (section == "Log" && key == "Format")
    {
        return "\tTEXT\n";
    }
    if (section == "Log" && key == "ToConsole")
    {
        return " off ";
    }
    return "";
}

std::string TelemetryGetter(const std::string& section, const std::string& key)
{
    if (section == "Telemetry" && key == "Enabled")
    {
        return " ON ";
    }
    if (section == "Telemetry" && key == "Protocol")
    {
        return " OTLP-GRPC ";
    }
    if (section == "Telemetry" && key == "SampleRatio")
    {
        return "1.5";
    }
    return "";
}
} // namespace

TEST(LoggingConfigTest, LogConfigUsesSharedModuleAlgorithmsForTrimAndLower)
{
    const memolog::LogConfig config = memolog::LogConfig::FromGetter(LoggingGetter);

    EXPECT_EQ(config.level, "warn");
    EXPECT_EQ(config.format, "text");
    EXPECT_FALSE(config.to_console);
}

TEST(LoggingConfigTest, TelemetryConfigUsesSharedModuleAlgorithmsForTrimAndLower)
{
    const memolog::TelemetryConfig config = memolog::TelemetryConfig::FromGetter(TelemetryGetter);

    EXPECT_TRUE(config.enabled);
    EXPECT_EQ(config.protocol, "otlp-grpc");
    EXPECT_DOUBLE_EQ(config.sample_ratio, 1.0);
}

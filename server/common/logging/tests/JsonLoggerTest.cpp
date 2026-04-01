#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "logging/Logger.h"
#include "logging/LogConfig.h"
#include <spdlog/spdlog.h>

#include <map>
#include <string>
#include <thread>
#include <future>
#include <cstdlib>

using namespace ::testing;
using namespace memolog;

LogConfig make_test_config() {
    LogConfig cfg;
    cfg.level = "debug";
    cfg.dir = "./logs";
    cfg.format = "json";
    cfg.to_console = false;
    cfg.rotate_mode = "daily";
    cfg.max_files = 3;
    cfg.max_size_mb = 10;
    cfg.redact = false;
    cfg.env = "test";
    return cfg;
}

// ---------------------------------------------------------------------------
// ParseLevel — returns a valid spdlog level and doesn't crash
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, ParseLevel_ReturnValueIsValid)
{
    auto l = ParseLevel("trace");
    EXPECT_GE(l, spdlog::level::trace);
    EXPECT_LE(l, spdlog::level::critical);

    l = ParseLevel("debug");
    EXPECT_GE(l, spdlog::level::trace);
    EXPECT_LE(l, spdlog::level::critical);

    l = ParseLevel("info");
    EXPECT_EQ(l, spdlog::level::info);

    l = ParseLevel("warn");
    EXPECT_EQ(l, spdlog::level::warn);

    l = ParseLevel("error");
    EXPECT_EQ(l, spdlog::level::err);

    l = ParseLevel("critical");
    EXPECT_EQ(l, spdlog::level::critical);
}

TEST(JsonLoggerTest, ParseLevel_UnknownStringDoesNotCrash)
{
    // Unknown strings must not throw — they fall back to info
    EXPECT_EQ(ParseLevel("DEBUG"), spdlog::level::info);
    EXPECT_EQ(ParseLevel("foobar"), spdlog::level::info);
    EXPECT_EQ(ParseLevel(""), spdlog::level::info);
    EXPECT_EQ(ParseLevel("warning"), spdlog::level::info);
    EXPECT_EQ(ParseLevel("err"), spdlog::level::info);
    EXPECT_EQ(ParseLevel("off"), spdlog::level::info);
}

// ---------------------------------------------------------------------------
// LogConfig defaults
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, LogConfig_HasSensibleDefaults)
{
    LogConfig cfg;
    EXPECT_EQ(cfg.level, "info");
    EXPECT_EQ(cfg.dir, "./logs");
    EXPECT_EQ(cfg.format, "json");
    EXPECT_TRUE(cfg.to_console);
    EXPECT_EQ(cfg.rotate_mode, "daily");
    EXPECT_EQ(cfg.max_files, 14);
    EXPECT_EQ(cfg.max_size_mb, 100);
    EXPECT_TRUE(cfg.redact);
    EXPECT_EQ(cfg.env, "local");
}

// ---------------------------------------------------------------------------
// Logger::Init and Shutdown
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, InitShutdown_Cycle)
{
    Logger::Init("test-service", make_test_config());
    ASSERT_TRUE(Logger::Get() != nullptr);
    EXPECT_EQ(Logger::ServiceName(), "test-service");
    EXPECT_NO_THROW(Logger::Shutdown());
    EXPECT_NO_THROW(Logger::Shutdown());
}

TEST(JsonLoggerTest, ServiceName_AfterInit)
{
    Logger::Init("MyService", make_test_config());
    EXPECT_EQ(Logger::ServiceName(), "MyService");
    Logger::Shutdown();
}

TEST(JsonLoggerTest, Config_AfterInit)
{
    Logger::Init("CfgTest", make_test_config());
    const LogConfig& stored = Logger::Config();
    EXPECT_EQ(stored.level, "debug");
    EXPECT_EQ(stored.env, "test");
    Logger::Shutdown();
}

// ---------------------------------------------------------------------------
// Log levels — macros don't crash
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, LogDebug_Reachable)
{
    Logger::Init("level-test", make_test_config());
    Logger::Get()->set_level(spdlog::level::debug);
    ASSERT_NO_THROW(LogDebug("debug_event", "a debug message"));
    EXPECT_NO_THROW(Logger::Shutdown());
}

TEST(JsonLoggerTest, LogInfo_Reachable)
{
    Logger::Init("level-test", make_test_config());
    ASSERT_NO_THROW(LogInfo("info_event", "an info message"));
    EXPECT_NO_THROW(Logger::Shutdown());
}

TEST(JsonLoggerTest, LogWarn_Reachable)
{
    Logger::Init("level-test", make_test_config());
    ASSERT_NO_THROW(LogWarn("warn_event", "a warning message"));
    EXPECT_NO_THROW(Logger::Shutdown());
}

TEST(JsonLoggerTest, LogError_Reachable)
{
    Logger::Init("level-test", make_test_config());
    ASSERT_NO_THROW(LogError("error_event", "an error message"));
    EXPECT_NO_THROW(Logger::Shutdown());
}

// ---------------------------------------------------------------------------
// Logger::Log accepts fields map
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, Log_AcceptsFieldsMap)
{
    Logger::Init("fields-test", make_test_config());
    std::map<std::string, std::string> fields = {
        {"user_id", "123"},
        {"action", "login"},
        {"duration_ms", "42"}
    };
    ASSERT_NO_THROW(Logger::Log(spdlog::level::info, "login_action",
                                "user logged in", fields));
    EXPECT_NO_THROW(Logger::Shutdown());
}

// ---------------------------------------------------------------------------
// Concurrent logging — no crash
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, ConcurrentLog_NoCrash)
{
    Logger::Init("concurrent-test", make_test_config());
    Logger::Get()->set_level(spdlog::level::info);

    std::vector<std::future<void>> futures;
    for (int i = 0; i < 4; ++i) {
        futures.emplace_back(std::async(std::launch::async, [i]() {
            for (int j = 0; j < 100; ++j) {
                std::map<std::string, std::string> f = {
                    {"thread_id", std::to_string(i)},
                    {"iter", std::to_string(j)}
                };
                Logger::Log(spdlog::level::info, "concurrent_event",
                            "thread " + std::to_string(i) + " iter " + std::to_string(j), f);
            }
        }));
    }
    for (auto& f : futures) f.get();

    EXPECT_NO_THROW(Logger::Shutdown());
}

// ---------------------------------------------------------------------------
// Re-initialization
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, ReInit_OverwritesPreviousLogger)
{
    Logger::Init("first", make_test_config());
    EXPECT_EQ(Logger::ServiceName(), "first");
    Logger::Init("second", make_test_config());
    EXPECT_EQ(Logger::ServiceName(), "second");
    Logger::Shutdown();
}

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "logging/Logger.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Redaction.hpp"
#include <spdlog/spdlog.h>

#include <map>
#include <string>
#include <thread>
#include <future>
#include <cstdlib>

using namespace ::testing;
using namespace memolog;

namespace memochat::tests::logging
{
bool ShouldUseDefaultLogDir(bool dir_empty);
bool IsLoggerTopLevelField(const char* key, unsigned long long size);
int ClassifySensitiveKey(const char* key, unsigned long long size);
bool ShouldCollapseTokenMask(unsigned long long size);
unsigned long long VisibleEmailLocalPrefix(unsigned long long local_size);
bool HasHttpSchemePrefix(const char* data, unsigned long long size);
bool HasExplicitEndpointPort(bool colon_found, unsigned long long colon_pos, unsigned long long authority_size);
bool IsParsedHttpEndpointOk(bool host_empty, bool target_empty);
bool ShouldUseFallbackTelemetryText(bool text_empty);
bool IsTelemetryEnabled(bool enabled, bool export_traces);
bool ShouldDropExportJob(bool endpoint_empty, bool body_empty);
bool ShouldKeepSpanAttribute(bool key_empty, bool value_empty);
bool ShouldInjectGrpcTraceId(bool trace_id_empty);
bool ShouldGenerateGrpcRequestId(bool request_id_empty);
bool ShouldInjectGrpcSpanId(bool span_id_empty);
bool ShouldGenerateBoundGrpcTraceId(bool metadata_trace_id_empty);
bool ShouldGenerateBoundGrpcRequestId(bool metadata_request_id_empty);
} // namespace memochat::tests::logging

LogConfig make_test_config()
{
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

TEST(JsonLoggerTest, LoggerAlgorithms_DefineDefaultDirAndTopLevelFields)
{
    EXPECT_TRUE(memochat::tests::logging::ShouldUseDefaultLogDir(true));
    EXPECT_FALSE(memochat::tests::logging::ShouldUseDefaultLogDir(false));
    EXPECT_TRUE(memochat::tests::logging::IsLoggerTopLevelField("service_instance", 16));
    EXPECT_TRUE(memochat::tests::logging::IsLoggerTopLevelField("duration_ms", 11));
    EXPECT_TRUE(memochat::tests::logging::IsLoggerTopLevelField("session_id", 10));
    EXPECT_FALSE(memochat::tests::logging::IsLoggerTopLevelField("display_name", 12));
    EXPECT_FALSE(memochat::tests::logging::IsLoggerTopLevelField(nullptr, 0));
}

TEST(JsonLoggerTest, TelemetryAlgorithms_DefineEndpointAndExportGuards)
{
    EXPECT_TRUE(memochat::tests::logging::HasHttpSchemePrefix("http://zipkin:9411/api/v2/spans", 31));
    EXPECT_FALSE(memochat::tests::logging::HasHttpSchemePrefix("https://zipkin:9411/api/v2/spans", 32));
    EXPECT_TRUE(memochat::tests::logging::HasExplicitEndpointPort(true, 6, 11));
    EXPECT_FALSE(memochat::tests::logging::HasExplicitEndpointPort(true, 10, 11));
    EXPECT_TRUE(memochat::tests::logging::IsParsedHttpEndpointOk(false, false));
    EXPECT_FALSE(memochat::tests::logging::IsParsedHttpEndpointOk(true, false));
    EXPECT_TRUE(memochat::tests::logging::ShouldUseFallbackTelemetryText(true));
    EXPECT_FALSE(memochat::tests::logging::ShouldUseFallbackTelemetryText(false));
    EXPECT_TRUE(memochat::tests::logging::IsTelemetryEnabled(true, true));
    EXPECT_FALSE(memochat::tests::logging::IsTelemetryEnabled(true, false));
    EXPECT_TRUE(memochat::tests::logging::ShouldDropExportJob(true, false));
    EXPECT_TRUE(memochat::tests::logging::ShouldDropExportJob(false, true));
    EXPECT_FALSE(memochat::tests::logging::ShouldDropExportJob(false, false));
    EXPECT_TRUE(memochat::tests::logging::ShouldKeepSpanAttribute(false, false));
    EXPECT_FALSE(memochat::tests::logging::ShouldKeepSpanAttribute(true, false));
    EXPECT_FALSE(memochat::tests::logging::ShouldKeepSpanAttribute(false, true));
}

TEST(JsonLoggerTest, GrpcTraceAlgorithms_DefineMetadataPropagationGuards)
{
    EXPECT_TRUE(memochat::tests::logging::ShouldInjectGrpcTraceId(false));
    EXPECT_FALSE(memochat::tests::logging::ShouldInjectGrpcTraceId(true));
    EXPECT_TRUE(memochat::tests::logging::ShouldGenerateGrpcRequestId(true));
    EXPECT_FALSE(memochat::tests::logging::ShouldGenerateGrpcRequestId(false));
    EXPECT_TRUE(memochat::tests::logging::ShouldInjectGrpcSpanId(false));
    EXPECT_FALSE(memochat::tests::logging::ShouldInjectGrpcSpanId(true));
    EXPECT_TRUE(memochat::tests::logging::ShouldGenerateBoundGrpcTraceId(true));
    EXPECT_FALSE(memochat::tests::logging::ShouldGenerateBoundGrpcTraceId(false));
    EXPECT_TRUE(memochat::tests::logging::ShouldGenerateBoundGrpcRequestId(true));
    EXPECT_FALSE(memochat::tests::logging::ShouldGenerateBoundGrpcRequestId(false));
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
    std::map<std::string, std::string> fields = {{"user_id", "123"}, {"action", "login"}, {"duration_ms", "42"}};
    ASSERT_NO_THROW(Logger::Log(spdlog::level::info, "login_action", "user logged in", fields));
    EXPECT_NO_THROW(Logger::Shutdown());
}

TEST(JsonLoggerTest, Redaction_UsesModuleBackedSensitiveKeyPolicy)
{
    EXPECT_NE(memochat::tests::logging::ClassifySensitiveKey("token", 5), 0);
    EXPECT_NE(memochat::tests::logging::ClassifySensitiveKey("email", 5), 0);
    EXPECT_EQ(memochat::tests::logging::ClassifySensitiveKey("display_name", 12), 0);
    EXPECT_TRUE(memochat::tests::logging::ShouldCollapseTokenMask(8));
    EXPECT_FALSE(memochat::tests::logging::ShouldCollapseTokenMask(9));
    EXPECT_EQ(memochat::tests::logging::VisibleEmailLocalPrefix(1), 1);
    EXPECT_EQ(memochat::tests::logging::VisibleEmailLocalPrefix(5), 2);

    EXPECT_TRUE(IsSensitiveKey("TOKEN"));
    EXPECT_TRUE(IsSensitiveKey("access_token"));
    EXPECT_TRUE(IsSensitiveKey("login_ticket"));
    EXPECT_TRUE(IsSensitiveKey("password_hash"));
    EXPECT_TRUE(IsSensitiveKey("pwd"));
    EXPECT_TRUE(IsSensitiveKey("api_key"));
    EXPECT_TRUE(IsSensitiveKey("client_secret"));
    EXPECT_TRUE(IsSensitiveKey("x-token"));
    EXPECT_TRUE(IsSensitiveKey("set-cookie"));
    EXPECT_TRUE(IsSensitiveKey("provider_token"));
    EXPECT_TRUE(IsSensitiveKey("Email"));
    EXPECT_TRUE(IsSensitiveKey("verify_code"));
    EXPECT_FALSE(IsSensitiveKey("display_name"));

    EXPECT_EQ(RedactValue("token", "12345678", true), "****");
    EXPECT_EQ(RedactValue("token", "123456789", true), "1234...6789");
    EXPECT_EQ(RedactValue("email", "a@example.com", true), "a***@example.com");
    EXPECT_EQ(RedactValue("email", "alice@example.com", true), "al***@example.com");
    EXPECT_EQ(RedactValue("password", "secret", true), "****");
    EXPECT_EQ(RedactValue("login_ticket", "ticket-value-1234", true), "tick...1234");
    EXPECT_EQ(RedactValue("api_key", "secret", true), "****");
    EXPECT_EQ(RedactValue("token", "123456789", false), "123456789");
}

// ---------------------------------------------------------------------------
// Concurrent logging — no crash
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, ConcurrentLog_NoCrash)
{
    Logger::Init("concurrent-test", make_test_config());
    Logger::Get()->set_level(spdlog::level::info);

    std::vector<std::future<void>> futures;
    for (int i = 0; i < 4; ++i)
    {
        futures.emplace_back(
            std::async(std::launch::async,
                       [i]()
                       {
                           for (int j = 0; j < 100; ++j)
                           {
                               std::map<std::string, std::string> f = {{"thread_id", std::to_string(i)},
                                                                       {"iter", std::to_string(j)}};
                               Logger::Log(spdlog::level::info,
                                           "concurrent_event",
                                           "thread " + std::to_string(i) + " iter " + std::to_string(j),
                                           f);
                           }
                       }));
    }
    for (auto& f : futures)
        f.get();

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

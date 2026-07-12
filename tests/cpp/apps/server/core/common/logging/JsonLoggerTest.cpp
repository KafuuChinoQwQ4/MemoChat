#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "logging/Logger.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Redaction.hpp"
#include "runtime/ExplicitThread.hpp"
#include <spdlog/spdlog.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

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

std::filesystem::path make_temp_log_path(const std::string& label)
{
    std::error_code error;
    const auto temp_dir = std::filesystem::temp_directory_path(error);
    if (error)
    {
        return {};
    }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    return temp_dir / ("memochat-logger-" + label + "-" + std::to_string(nonce));
}

std::string read_file(const std::filesystem::path& path)
{
    std::FILE* input = std::fopen(path.string().c_str(), "rb");
    if (input == nullptr)
    {
        return {};
    }

    std::string contents;
    char buffer[4096]{};
    for (;;)
    {
        const std::size_t count = std::fread(buffer, 1, sizeof(buffer), input);
        if (count != 0)
        {
            contents.append(buffer, count);
        }
        if (count != sizeof(buffer))
        {
            break;
        }
    }
    (void) std::fclose(input);
    return contents;
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
    // Unknown strings use the info fallback.
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
    ASSERT_TRUE(Logger::Init("test-service", make_test_config()));
    ASSERT_TRUE(Logger::Get() != nullptr);
    EXPECT_EQ(Logger::ServiceName(), "test-service");
    Logger::Shutdown();
    Logger::Shutdown();
}

TEST(JsonLoggerTest, InitReturnsFalseWhenLogDirIsARegularFile)
{
    const auto regular_file = make_temp_log_path("not-a-directory");
    ASSERT_FALSE(regular_file.empty());
    std::FILE* marker = std::fopen(regular_file.string().c_str(), "wb");
    ASSERT_NE(marker, nullptr);
    ASSERT_NE(std::fputs("not a directory", marker), EOF);
    ASSERT_EQ(std::fclose(marker), 0);

    auto cfg = make_test_config();
    cfg.dir = regular_file.string();
    std::string error;
    EXPECT_FALSE(Logger::Init("invalid-dir", cfg, &error));
    EXPECT_THAT(error, HasSubstr("log directory"));
    ASSERT_NE(Logger::Get(), nullptr);
    LogError("fallback_event", "file logger initialization failed");
    Logger::Shutdown();

    std::error_code path_error;
    std::filesystem::remove(regular_file, path_error);
    EXPECT_FALSE(path_error) << path_error.message();
}

TEST(JsonLoggerTest, FileSinkWritesOneJsonObjectPerPhysicalLine)
{
    const auto log_dir = make_temp_log_path("json-lines");
    ASSERT_FALSE(log_dir.empty());

    auto cfg = make_test_config();
    cfg.dir = log_dir.string();
    cfg.rotate_mode = "size";
    ASSERT_TRUE(Logger::Init("json-lines", cfg));
    LogInfo("first_line", "embedded\nnewline");
    LogInfo("second_line", "plain message");
    Logger::Shutdown();

    const std::string contents = read_file(log_dir / "json-lines.log.json");
    ASSERT_FALSE(contents.empty());
    EXPECT_EQ(std::count(contents.begin(), contents.end(), '\n'), 2);
    EXPECT_EQ(contents.front(), '{');
    EXPECT_EQ(contents.back(), '\n');
    EXPECT_THAT(contents, HasSubstr("\"event\":\"first_line\""));
    EXPECT_THAT(contents, HasSubstr("\"event\":\"second_line\""));
    EXPECT_THAT(contents, HasSubstr("embedded\\nnewline"));

    std::error_code cleanup_error;
    (void) std::filesystem::remove_all(log_dir, cleanup_error);
    EXPECT_FALSE(cleanup_error) << cleanup_error.message();
}

TEST(JsonLoggerTest, DailyModeUsesDatedFileName)
{
    const auto log_dir = make_temp_log_path("daily");
    ASSERT_FALSE(log_dir.empty());

    auto cfg = make_test_config();
    cfg.dir = log_dir.string();
    cfg.rotate_mode = "daily";
    ASSERT_TRUE(Logger::Init("daily", cfg));
    LogInfo("daily_event", "daily message");
    Logger::Shutdown();

    bool found_daily_log = false;
    std::error_code iteration_error;
    std::filesystem::directory_iterator entry(log_dir, iteration_error);
    const std::filesystem::directory_iterator end;
    while (!iteration_error && entry != end)
    {
        const std::string filename = entry->path().filename().string();
        found_daily_log = found_daily_log || (filename.starts_with("daily.log_") && filename.ends_with(".json"));
        entry.increment(iteration_error);
    }
    EXPECT_FALSE(iteration_error) << iteration_error.message();
    EXPECT_TRUE(found_daily_log);

    std::error_code cleanup_error;
    (void) std::filesystem::remove_all(log_dir, cleanup_error);
    EXPECT_FALSE(cleanup_error) << cleanup_error.message();
}

TEST(JsonLoggerTest, SizeModeKeepsConfiguredArchiveCount)
{
    const auto log_dir = make_temp_log_path("size-rotation");
    ASSERT_FALSE(log_dir.empty());

    auto cfg = make_test_config();
    cfg.dir = log_dir.string();
    cfg.rotate_mode = "size";
    cfg.max_files = 1;
    cfg.max_size_mb = 1;
    ASSERT_TRUE(Logger::Init("rotate", cfg));

    const std::string payload(700U * 1024U, 'x');
    LogInfo("first_segment", payload);
    LogInfo("second_segment", payload);
    LogInfo("third_segment", payload);
    Logger::Shutdown();

    std::error_code path_error;
    EXPECT_TRUE(std::filesystem::exists(log_dir / "rotate.log.json", path_error));
    EXPECT_FALSE(path_error) << path_error.message();
    EXPECT_TRUE(std::filesystem::exists(log_dir / "rotate.log.1.json", path_error));
    EXPECT_FALSE(path_error) << path_error.message();
    EXPECT_FALSE(std::filesystem::exists(log_dir / "rotate.log.2.json", path_error));
    EXPECT_FALSE(path_error) << path_error.message();
    EXPECT_THAT(read_file(log_dir / "rotate.log.1.json"), HasSubstr("\"event\":\"second_segment\""));
    EXPECT_THAT(read_file(log_dir / "rotate.log.json"), HasSubstr("\"event\":\"third_segment\""));

    std::error_code cleanup_error;
    (void) std::filesystem::remove_all(log_dir, cleanup_error);
    EXPECT_FALSE(cleanup_error) << cleanup_error.message();
}

TEST(JsonLoggerTest, ServiceName_AfterInit)
{
    ASSERT_TRUE(Logger::Init("MyService", make_test_config()));
    EXPECT_EQ(Logger::ServiceName(), "MyService");
    Logger::Shutdown();
}

TEST(JsonLoggerTest, Config_AfterInit)
{
    ASSERT_TRUE(Logger::Init("CfgTest", make_test_config()));
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
    ASSERT_TRUE(Logger::Init("level-test", make_test_config()));
    Logger::Get()->set_level(spdlog::level::debug);
    LogDebug("debug_event", "a debug message");
    Logger::Shutdown();
}

TEST(JsonLoggerTest, LogInfo_Reachable)
{
    ASSERT_TRUE(Logger::Init("level-test", make_test_config()));
    LogInfo("info_event", "an info message");
    Logger::Shutdown();
}

TEST(JsonLoggerTest, LogWarn_Reachable)
{
    ASSERT_TRUE(Logger::Init("level-test", make_test_config()));
    LogWarn("warn_event", "a warning message");
    Logger::Shutdown();
}

TEST(JsonLoggerTest, LogError_Reachable)
{
    ASSERT_TRUE(Logger::Init("level-test", make_test_config()));
    LogError("error_event", "an error message");
    Logger::Shutdown();
}

// ---------------------------------------------------------------------------
// Logger::Log accepts fields map
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, Log_AcceptsFieldsMap)
{
    ASSERT_TRUE(Logger::Init("fields-test", make_test_config()));
    std::map<std::string, std::string> fields = {{"user_id", "123"}, {"action", "login"}, {"duration_ms", "42"}};
    Logger::Log(spdlog::level::info, "login_action", "user logged in", fields);
    Logger::Shutdown();
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
    ASSERT_TRUE(Logger::Init("concurrent-test", make_test_config()));
    Logger::Get()->set_level(spdlog::level::info);

    std::vector<memochat::runtime::ExplicitThread> threads(4);
    for (int i = 0; i < 4; ++i)
    {
        std::string error;
        ASSERT_TRUE(threads[static_cast<std::size_t>(i)].Start(
            [i]() noexcept
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
            },
            &error))
            << error;
    }
    for (auto& thread : threads)
    {
        std::string error;
        ASSERT_TRUE(thread.Join(&error)) << error;
    }

    Logger::Shutdown();
}

// ---------------------------------------------------------------------------
// Re-initialization
// ---------------------------------------------------------------------------
TEST(JsonLoggerTest, ReInit_OverwritesPreviousLogger)
{
    ASSERT_TRUE(Logger::Init("first", make_test_config()));
    EXPECT_EQ(Logger::ServiceName(), "first");
    ASSERT_TRUE(Logger::Init("second", make_test_config()));
    EXPECT_EQ(Logger::ServiceName(), "second");
    Logger::Shutdown();
}

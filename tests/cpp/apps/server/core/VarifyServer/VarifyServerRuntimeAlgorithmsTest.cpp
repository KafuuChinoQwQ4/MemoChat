#include <gtest/gtest.h>

#include <string_view>

namespace memochat::tests::varify::server
{
int DefaultHealthPort();
const char* HealthPath();
const char* ReadinessPath();
const char* TextContentType();
const char* JsonContentType();
const char* NotFoundBody();
const char* RedisDownBody();
const char* ReadyBody();
const char* HealthBody();
bool IsHealthPath(const char* target, unsigned long target_size);
bool IsReadinessPath(const char* target, unsigned long target_size);
bool ShouldReplyNotFound(bool is_health, bool is_ready);
bool ShouldReportRedisDown(bool is_ready, bool redis_ok);
bool ShouldUseConfiguredHealthPort(bool health_port_text_empty);
} // namespace memochat::tests::varify::server

namespace
{
bool IsHealthPath(std::string_view target)
{
    return memochat::tests::varify::server::IsHealthPath(target.data(), target.size());
}

bool IsReadinessPath(std::string_view target)
{
    return memochat::tests::varify::server::IsReadinessPath(target.data(), target.size());
}
} // namespace

TEST(VarifyServerRuntimeAlgorithmsTest, ExposesStableHealthMetadata)
{
    EXPECT_EQ(memochat::tests::varify::server::DefaultHealthPort(), 8081);
    EXPECT_STREQ(memochat::tests::varify::server::HealthPath(), "/healthz");
    EXPECT_STREQ(memochat::tests::varify::server::ReadinessPath(), "/readyz");
    EXPECT_STREQ(memochat::tests::varify::server::TextContentType(), "text/plain");
    EXPECT_STREQ(memochat::tests::varify::server::JsonContentType(), "application/json");
}

TEST(VarifyServerRuntimeAlgorithmsTest, MatchesOnlyExactProbePaths)
{
    EXPECT_TRUE(IsHealthPath("/healthz"));
    EXPECT_TRUE(IsReadinessPath("/readyz"));

    EXPECT_FALSE(IsHealthPath("/healthz/extra"));
    EXPECT_FALSE(IsHealthPath("/readyz"));
    EXPECT_FALSE(IsHealthPath(""));
    EXPECT_FALSE(memochat::tests::varify::server::IsHealthPath(nullptr, 0));

    EXPECT_FALSE(IsReadinessPath("/readyz?full=1"));
    EXPECT_FALSE(IsReadinessPath("/healthz"));
    EXPECT_FALSE(IsReadinessPath(""));
    EXPECT_FALSE(memochat::tests::varify::server::IsReadinessPath(nullptr, 0));
}

TEST(VarifyServerRuntimeAlgorithmsTest, KeepsHealthResponseBodiesStable)
{
    EXPECT_STREQ(memochat::tests::varify::server::NotFoundBody(), "not found");
    EXPECT_STREQ(memochat::tests::varify::server::RedisDownBody(),
                 R"({"status":"unhealthy","reason":"redis_down","service":"VarifyServer"})");
    EXPECT_STREQ(memochat::tests::varify::server::ReadyBody(), R"({"status":"ready","service":"VarifyServer"})");
    EXPECT_STREQ(memochat::tests::varify::server::HealthBody(), R"({"status":"ok","service":"VarifyServer"})");
}

TEST(VarifyServerRuntimeAlgorithmsTest, ExposesBranchGuards)
{
    EXPECT_TRUE(memochat::tests::varify::server::ShouldReplyNotFound(false, false));
    EXPECT_FALSE(memochat::tests::varify::server::ShouldReplyNotFound(true, false));
    EXPECT_FALSE(memochat::tests::varify::server::ShouldReplyNotFound(false, true));

    EXPECT_TRUE(memochat::tests::varify::server::ShouldReportRedisDown(true, false));
    EXPECT_FALSE(memochat::tests::varify::server::ShouldReportRedisDown(true, true));
    EXPECT_FALSE(memochat::tests::varify::server::ShouldReportRedisDown(false, false));

    EXPECT_TRUE(memochat::tests::varify::server::ShouldUseConfiguredHealthPort(false));
    EXPECT_FALSE(memochat::tests::varify::server::ShouldUseConfiguredHealthPort(true));
}

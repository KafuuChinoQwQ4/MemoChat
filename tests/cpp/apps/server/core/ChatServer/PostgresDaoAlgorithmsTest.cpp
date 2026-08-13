#include <gtest/gtest.h>

bool MemoChatTestPostgresDaoUsesFallbackSection(bool primary_host_empty, bool fallback_section_empty);
bool MemoChatTestPostgresDaoHasPostgresHost(bool host_empty);
const char* MemoChatTestPostgresDaoDefaultSslMode();
const char* MemoChatTestPostgresDaoDefaultSchema();
const char* MemoChatTestPostgresDaoSelectSslMode(bool sslmode_empty, const char* sslmode);
const char* MemoChatTestPostgresDaoSelectSchema(bool schema_empty, const char* schema);
int MemoChatTestPostgresDaoConnectTimeoutSeconds();
int MemoChatTestPostgresDaoStatementTimeoutMilliseconds();
int MemoChatTestPostgresDaoHealthProbeIntervalMilliseconds();
int MemoChatTestPostgresDaoTcpUserTimeoutMilliseconds();
int MemoChatTestPostgresDaoHealthProbeDeadlineMilliseconds();
const char* MemoChatTestPostgresDaoHealthProbeSessionOptions();

TEST(PostgresDaoAlgorithmsTest, SelectsFallbackSectionOnlyWhenPrimaryHostIsMissing)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoUsesFallbackSection(true, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsesFallbackSection(false, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsesFallbackSection(true, true));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsesFallbackSection(false, true));
}

TEST(PostgresDaoAlgorithmsTest, GatesPostgresHost)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoHasPostgresHost(false));
    EXPECT_FALSE(MemoChatTestPostgresDaoHasPostgresHost(true));
}

TEST(PostgresDaoAlgorithmsTest, SelectsConnectionDefaults)
{
    EXPECT_STREQ("disable", MemoChatTestPostgresDaoDefaultSslMode());
    EXPECT_STREQ("public", MemoChatTestPostgresDaoDefaultSchema());

    EXPECT_STREQ("disable", MemoChatTestPostgresDaoSelectSslMode(true, ""));
    EXPECT_STREQ("disable", MemoChatTestPostgresDaoSelectSslMode(true, nullptr));
    EXPECT_STREQ("require", MemoChatTestPostgresDaoSelectSslMode(false, "require"));

    EXPECT_STREQ("public", MemoChatTestPostgresDaoSelectSchema(true, ""));
    EXPECT_STREQ("public", MemoChatTestPostgresDaoSelectSchema(true, nullptr));
    EXPECT_STREQ("memo_chat", MemoChatTestPostgresDaoSelectSchema(false, "memo_chat"));
}

TEST(PostgresDaoAlgorithmsTest, BoundsBackgroundHealthChecks)
{
    EXPECT_EQ(MemoChatTestPostgresDaoConnectTimeoutSeconds(), 3);
    EXPECT_EQ(MemoChatTestPostgresDaoStatementTimeoutMilliseconds(), 2000);
    EXPECT_EQ(MemoChatTestPostgresDaoHealthProbeIntervalMilliseconds(), 10000);
    EXPECT_EQ(MemoChatTestPostgresDaoTcpUserTimeoutMilliseconds(), 3000);
    EXPECT_EQ(MemoChatTestPostgresDaoHealthProbeDeadlineMilliseconds(), 3000);
    EXPECT_STREQ(MemoChatTestPostgresDaoHealthProbeSessionOptions(), "-c statement_timeout=2000");
}

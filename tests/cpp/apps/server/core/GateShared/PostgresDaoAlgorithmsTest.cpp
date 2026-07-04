#include <gtest/gtest.h>

namespace memochat::tests::gate::postgres_dao
{
const char* DefaultSchema();
const char* DefaultSslMode();
int ConnectTimeoutSeconds();
int DefaultPoolSize();
int MinPoolSize();
int MaxPoolSize();
int NormalizePoolSize(bool configured_empty, int parsed_pool_size);
bool ShouldFetchUserProfiles(bool connection_string_empty, bool uid_list_empty);
bool ShouldUseAccountPostgresSection(bool account_host_empty);
} // namespace memochat::tests::gate::postgres_dao

TEST(PostgresDaoAlgorithmsTest, ExposesConnectionDefaults)
{
    using namespace memochat::tests::gate::postgres_dao;

    EXPECT_STREQ(DefaultSchema(), "public");
    EXPECT_STREQ(DefaultSslMode(), "disable");
    EXPECT_EQ(ConnectTimeoutSeconds(), 3);
}

TEST(PostgresDaoAlgorithmsTest, NormalizesPoolSize)
{
    using namespace memochat::tests::gate::postgres_dao;

    EXPECT_EQ(DefaultPoolSize(), 12);
    EXPECT_EQ(MinPoolSize(), 1);
    EXPECT_EQ(MaxPoolSize(), 64);
    EXPECT_EQ(NormalizePoolSize(true, 0), 12);
    EXPECT_EQ(NormalizePoolSize(false, -1), 1);
    EXPECT_EQ(NormalizePoolSize(false, 0), 1);
    EXPECT_EQ(NormalizePoolSize(false, 7), 7);
    EXPECT_EQ(NormalizePoolSize(false, 65), 64);
}

TEST(PostgresDaoAlgorithmsTest, ClassifiesOptionalAccountPostgresAndProfileFetch)
{
    using namespace memochat::tests::gate::postgres_dao;

    EXPECT_FALSE(ShouldUseAccountPostgresSection(true));
    EXPECT_TRUE(ShouldUseAccountPostgresSection(false));
    EXPECT_FALSE(ShouldFetchUserProfiles(true, false));
    EXPECT_FALSE(ShouldFetchUserProfiles(false, true));
    EXPECT_TRUE(ShouldFetchUserProfiles(false, false));
}

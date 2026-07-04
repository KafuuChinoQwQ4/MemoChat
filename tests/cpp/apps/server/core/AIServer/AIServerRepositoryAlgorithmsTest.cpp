#include <gtest/gtest.h>

namespace memochat::tests::ai::repository
{
const char* DefaultPostgresSchema();
bool ShouldUseDefaultSchema(bool schema_empty);
const char* DefaultSessionTitle();
bool HasAffectedRows(unsigned long long affected_rows);
bool ShouldReturnNoSession(bool rows_empty);
} // namespace memochat::tests::ai::repository

TEST(AIServerRepositoryAlgorithmsTest, ExposesPostgresSchemaDefaults)
{
    using namespace memochat::tests::ai::repository;

    EXPECT_STREQ(DefaultPostgresSchema(), "public");
    EXPECT_TRUE(ShouldUseDefaultSchema(true));
    EXPECT_FALSE(ShouldUseDefaultSchema(false));
}

TEST(AIServerRepositoryAlgorithmsTest, ExposesSessionAndMutationGuards)
{
    using namespace memochat::tests::ai::repository;

    EXPECT_STREQ(DefaultSessionTitle(), "");
    EXPECT_FALSE(HasAffectedRows(0));
    EXPECT_TRUE(HasAffectedRows(1));
    EXPECT_TRUE(HasAffectedRows(2));
    EXPECT_TRUE(ShouldReturnNoSession(true));
    EXPECT_FALSE(ShouldReturnNoSession(false));
}

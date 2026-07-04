#include <gtest/gtest.h>

namespace memochat::tests::gate::postgres_dao_account
{
const char* DefaultSchema();
const char* DefaultUserIcon();
int UserPublicIdMaxAttempts();
bool ShouldAcceptGeneratedPublicId(bool rows_empty);
bool HasGeneratedUserPublicId(bool public_id_empty);
bool HasPositiveUid(int uid);
bool IsAffectedRowsPositive(unsigned long long affected_rows);
bool ShouldUseAccountBridge(bool account_connection_empty);
} // namespace memochat::tests::gate::postgres_dao_account

TEST(PostgresDaoAccountAlgorithmsTest, ExposesStableDefaults)
{
    using namespace memochat::tests::gate::postgres_dao_account;

    EXPECT_STREQ(DefaultSchema(), "public");
    EXPECT_STREQ(DefaultUserIcon(), ":/res/head_1.jpg");
    EXPECT_EQ(UserPublicIdMaxAttempts(), 20);
}

TEST(PostgresDaoAccountAlgorithmsTest, ClassifiesRegistrationAndAccountLookupGuards)
{
    using namespace memochat::tests::gate::postgres_dao_account;

    EXPECT_TRUE(ShouldAcceptGeneratedPublicId(true));
    EXPECT_FALSE(ShouldAcceptGeneratedPublicId(false));
    EXPECT_TRUE(HasGeneratedUserPublicId(false));
    EXPECT_FALSE(HasGeneratedUserPublicId(true));
    EXPECT_FALSE(HasPositiveUid(0));
    EXPECT_FALSE(HasPositiveUid(-1));
    EXPECT_TRUE(HasPositiveUid(1));
    EXPECT_FALSE(IsAffectedRowsPositive(0));
    EXPECT_TRUE(IsAffectedRowsPositive(1));
    EXPECT_FALSE(ShouldUseAccountBridge(true));
    EXPECT_TRUE(ShouldUseAccountBridge(false));
}

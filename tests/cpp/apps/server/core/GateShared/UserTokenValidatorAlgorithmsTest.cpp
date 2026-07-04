#include <gtest/gtest.h>

namespace memochat::tests::gate::auth
{
bool ShouldValidateUserToken(int uid, bool token_empty);
bool ShouldAcceptStoredUserToken(bool redis_hit, bool stored_token_empty, bool token_matches);
} // namespace memochat::tests::gate::auth

TEST(UserTokenValidatorAlgorithmsTest, GuardsInvalidUidAndEmptyTokenBeforeRedisLookup)
{
    EXPECT_FALSE(memochat::tests::gate::auth::ShouldValidateUserToken(0, false));
    EXPECT_FALSE(memochat::tests::gate::auth::ShouldValidateUserToken(-5, false));
    EXPECT_FALSE(memochat::tests::gate::auth::ShouldValidateUserToken(42, true));
    EXPECT_TRUE(memochat::tests::gate::auth::ShouldValidateUserToken(42, false));
}

TEST(UserTokenValidatorAlgorithmsTest, AcceptsOnlyRedisHitWithNonEmptyMatchingValue)
{
    EXPECT_FALSE(memochat::tests::gate::auth::ShouldAcceptStoredUserToken(false, false, true));
    EXPECT_FALSE(memochat::tests::gate::auth::ShouldAcceptStoredUserToken(true, true, true));
    EXPECT_FALSE(memochat::tests::gate::auth::ShouldAcceptStoredUserToken(true, false, false));
    EXPECT_TRUE(memochat::tests::gate::auth::ShouldAcceptStoredUserToken(true, false, true));
}

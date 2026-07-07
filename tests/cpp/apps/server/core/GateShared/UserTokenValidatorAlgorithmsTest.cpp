#include <gtest/gtest.h>

namespace memochat::tests::gate::auth
{
bool ShouldValidateUserToken(int uid, bool token_empty);
bool ShouldAcceptStoredUserToken(bool redis_hit, bool stored_token_empty, bool token_matches);
bool ShouldAcceptJwtAccessClaims(bool jwt_valid, int expected_uid, int claims_uid);
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

TEST(UserTokenValidatorAlgorithmsTest, AcceptsOnlyValidJwtClaimsForExpectedUid)
{
    EXPECT_FALSE(memochat::tests::gate::auth::ShouldAcceptJwtAccessClaims(false, 42, 42));
    EXPECT_FALSE(memochat::tests::gate::auth::ShouldAcceptJwtAccessClaims(true, 0, 42));
    EXPECT_FALSE(memochat::tests::gate::auth::ShouldAcceptJwtAccessClaims(true, 42, 43));
    EXPECT_TRUE(memochat::tests::gate::auth::ShouldAcceptJwtAccessClaims(true, 42, 42));
}

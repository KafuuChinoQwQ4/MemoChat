#include <gtest/gtest.h>

namespace memochat::tests::account::auth_cache
{
const char* VerificationCodePrefix();
const char* HttpTokenPrefix();
const char* LoginProfilePrefix();
const char* LoginProfileUidPrefix();
} // namespace memochat::tests::account::auth_cache

TEST(AuthCacheAlgorithmsTest, ExposesStableRedisKeyPrefixes)
{
    using namespace memochat::tests::account::auth_cache;

    EXPECT_STREQ(VerificationCodePrefix(), "code_");
    EXPECT_STREQ(HttpTokenPrefix(), "utoken_");
    EXPECT_STREQ(LoginProfilePrefix(), "ulogin_profile_");
    EXPECT_STREQ(LoginProfileUidPrefix(), "ulogin_profile_uid_");
}

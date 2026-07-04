#include <gtest/gtest.h>

namespace memochat::tests::gate::redis_pipeline
{
const char* LoginProfilePrefix();
const char* LoginProfileUidPrefix();
const char* UserTokenPrefix();
} // namespace memochat::tests::gate::redis_pipeline

TEST(RedisPipelineAlgorithmsTest, ExposesStableLoginCacheKeyPrefixes)
{
    using namespace memochat::tests::gate::redis_pipeline;

    EXPECT_STREQ(LoginProfilePrefix(), "ulogin_profile_");
    EXPECT_STREQ(LoginProfileUidPrefix(), "ulogin_profile_uid_");
    EXPECT_STREQ(UserTokenPrefix(), "utoken_");
}

#include <gtest/gtest.h>

namespace memochat::tests::varify::rate_limiter
{
const char* EmailRateLimitPrefix();
const char* IpRateLimitPrefix();
bool IsRedisCounterError(long long count);
bool ShouldRateLimit(long long count, int max_requests);
bool ShouldAllowRequest(long long count, int max_requests);
} // namespace memochat::tests::varify::rate_limiter

TEST(RateLimiterAlgorithmsTest, ExposesStableRedisKeyPrefixes)
{
    EXPECT_STREQ(memochat::tests::varify::rate_limiter::EmailRateLimitPrefix(), "varify_rl_email:");
    EXPECT_STREQ(memochat::tests::varify::rate_limiter::IpRateLimitPrefix(), "varify_rl_ip:");
}

TEST(RateLimiterAlgorithmsTest, ClassifiesRedisCounterErrors)
{
    EXPECT_TRUE(memochat::tests::varify::rate_limiter::IsRedisCounterError(-1));
    EXPECT_FALSE(memochat::tests::varify::rate_limiter::IsRedisCounterError(0));
    EXPECT_FALSE(memochat::tests::varify::rate_limiter::IsRedisCounterError(1));
}

TEST(RateLimiterAlgorithmsTest, PreservesExistingLimitBoundary)
{
    EXPECT_FALSE(memochat::tests::varify::rate_limiter::ShouldRateLimit(3, 3));
    EXPECT_TRUE(memochat::tests::varify::rate_limiter::ShouldRateLimit(4, 3));

    EXPECT_TRUE(memochat::tests::varify::rate_limiter::ShouldAllowRequest(3, 3));
    EXPECT_FALSE(memochat::tests::varify::rate_limiter::ShouldAllowRequest(4, 3));
    EXPECT_FALSE(memochat::tests::varify::rate_limiter::ShouldAllowRequest(-1, 3));
}

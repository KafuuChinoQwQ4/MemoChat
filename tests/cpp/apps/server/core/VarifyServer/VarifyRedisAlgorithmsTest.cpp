#include <gtest/gtest.h>

namespace memochat::tests::varify::redis
{
int DefaultRedisPort();
int DefaultConnectionPoolSize();
bool HasPassword(char first_char);
bool HasReply(bool reply_present);
bool IsExpectedReplyType(int actual_type, int expected_type);
bool ShouldDropPoolConnection(bool reply_present, int context_error);
long long RedisCounterError();
long long RedisMissingTtl();
bool IsFirstIncrement(long long count);
} // namespace memochat::tests::varify::redis

TEST(VarifyRedisAlgorithmsTest, ExposesStableRedisDefaults)
{
    EXPECT_EQ(memochat::tests::varify::redis::DefaultRedisPort(), 6379);
    EXPECT_EQ(memochat::tests::varify::redis::DefaultConnectionPoolSize(), 5);
}

TEST(VarifyRedisAlgorithmsTest, PreservesPasswordAndReplyGuards)
{
    EXPECT_FALSE(memochat::tests::varify::redis::HasPassword('\0'));
    EXPECT_TRUE(memochat::tests::varify::redis::HasPassword('x'));

    EXPECT_FALSE(memochat::tests::varify::redis::HasReply(false));
    EXPECT_TRUE(memochat::tests::varify::redis::HasReply(true));

    EXPECT_TRUE(memochat::tests::varify::redis::IsExpectedReplyType(3, 3));
    EXPECT_FALSE(memochat::tests::varify::redis::IsExpectedReplyType(2, 3));
}

TEST(VarifyRedisAlgorithmsTest, PreservesPoolDropAndSentinelBoundaries)
{
    EXPECT_FALSE(memochat::tests::varify::redis::ShouldDropPoolConnection(true, 0));
    EXPECT_TRUE(memochat::tests::varify::redis::ShouldDropPoolConnection(false, 0));
    EXPECT_TRUE(memochat::tests::varify::redis::ShouldDropPoolConnection(true, 1));

    EXPECT_EQ(memochat::tests::varify::redis::RedisCounterError(), -1);
    EXPECT_EQ(memochat::tests::varify::redis::RedisMissingTtl(), -2);

    EXPECT_TRUE(memochat::tests::varify::redis::IsFirstIncrement(1));
    EXPECT_FALSE(memochat::tests::varify::redis::IsFirstIncrement(0));
    EXPECT_FALSE(memochat::tests::varify::redis::IsFirstIncrement(2));
}

#include <gtest/gtest.h>

bool MemoChatTestInlineTaskAcceptsAllRoutingKeys(bool routing_keys_empty);
bool MemoChatTestInlineTaskShouldConsume(long long available_at_ms, long long now_ms);
bool MemoChatTestInlineTaskShouldDropAfterRetry(int retry_count, int max_retries);
int MemoChatTestInlineTaskRetryDelayMs();

TEST(InlineTaskBusAlgorithmsTest, AcceptsAllRoutingKeysOnlyForEmptyFilter)
{
    EXPECT_TRUE(MemoChatTestInlineTaskAcceptsAllRoutingKeys(true));
    EXPECT_FALSE(MemoChatTestInlineTaskAcceptsAllRoutingKeys(false));
}

TEST(InlineTaskBusAlgorithmsTest, ConsumesOnlyAvailableTasks)
{
    EXPECT_TRUE(MemoChatTestInlineTaskShouldConsume(1000, 1000));
    EXPECT_TRUE(MemoChatTestInlineTaskShouldConsume(999, 1000));
    EXPECT_FALSE(MemoChatTestInlineTaskShouldConsume(1001, 1000));
}

TEST(InlineTaskBusAlgorithmsTest, DropsOnlyAfterRetryLimitIsExceeded)
{
    EXPECT_FALSE(MemoChatTestInlineTaskShouldDropAfterRetry(0, 0));
    EXPECT_FALSE(MemoChatTestInlineTaskShouldDropAfterRetry(3, 3));
    EXPECT_TRUE(MemoChatTestInlineTaskShouldDropAfterRetry(4, 3));
}

TEST(InlineTaskBusAlgorithmsTest, KeepsLegacyRetryDelay)
{
    EXPECT_EQ(MemoChatTestInlineTaskRetryDelayMs(), 1000);
}

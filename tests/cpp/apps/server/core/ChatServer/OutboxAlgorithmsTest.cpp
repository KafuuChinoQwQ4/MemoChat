#include <gtest/gtest.h>

int MemoChatTestOutboxNextRetryCount(int retry_count);
int MemoChatTestOutboxSelectBackoffMs(int retry_base_ms, int retry_max_ms, int retry_count);
bool MemoChatTestOutboxTerminalRetry(int retry_count, int retry_max);
bool MemoChatTestOutboxShouldScheduleRepairTask(bool terminal_retry, bool has_repair_task_publisher);

TEST(OutboxAlgorithmsTest, AdvancesRetryCount)
{
    EXPECT_EQ(MemoChatTestOutboxNextRetryCount(0), 1);
    EXPECT_EQ(MemoChatTestOutboxNextRetryCount(4), 5);
}

TEST(OutboxAlgorithmsTest, SelectsClampedLinearBackoff)
{
    EXPECT_EQ(MemoChatTestOutboxSelectBackoffMs(1000, 30000, 0), 1000);
    EXPECT_EQ(MemoChatTestOutboxSelectBackoffMs(1000, 30000, 2), 2000);
    EXPECT_EQ(MemoChatTestOutboxSelectBackoffMs(10000, 30000, 5), 30000);
}

TEST(OutboxAlgorithmsTest, DetectsTerminalRetry)
{
    EXPECT_FALSE(MemoChatTestOutboxTerminalRetry(4, 5));
    EXPECT_TRUE(MemoChatTestOutboxTerminalRetry(5, 5));
    EXPECT_TRUE(MemoChatTestOutboxTerminalRetry(6, 5));
}

TEST(OutboxAlgorithmsTest, SchedulesRepairOnlyForNonTerminalRetryWithPublisher)
{
    EXPECT_TRUE(MemoChatTestOutboxShouldScheduleRepairTask(false, true));
    EXPECT_FALSE(MemoChatTestOutboxShouldScheduleRepairTask(false, false));
    EXPECT_FALSE(MemoChatTestOutboxShouldScheduleRepairTask(true, true));
}

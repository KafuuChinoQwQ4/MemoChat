#include <gtest/gtest.h>

bool MemoChatTestDeliveryRuntimeInitialStartedExpected();
bool MemoChatTestDeliveryRuntimeStopRequestedWhenStarting();
bool MemoChatTestDeliveryRuntimeStopRequestedWhenStopping();
bool MemoChatTestDeliveryRuntimeStartedAfterStopAndJoin();
bool MemoChatTestDeliveryRuntimeShouldJoinThread(bool joinable);
bool MemoChatTestDeliveryRuntimeShouldRunLoop(bool has_loop);

TEST(DeliveryRuntimeAlgorithmsTest, KeepsLifecycleFlagTransitions)
{
    EXPECT_FALSE(MemoChatTestDeliveryRuntimeInitialStartedExpected());
    EXPECT_FALSE(MemoChatTestDeliveryRuntimeStopRequestedWhenStarting());
    EXPECT_TRUE(MemoChatTestDeliveryRuntimeStopRequestedWhenStopping());
    EXPECT_FALSE(MemoChatTestDeliveryRuntimeStartedAfterStopAndJoin());
}

TEST(DeliveryRuntimeAlgorithmsTest, MirrorsThreadJoinableGuard)
{
    EXPECT_TRUE(MemoChatTestDeliveryRuntimeShouldJoinThread(true));
    EXPECT_FALSE(MemoChatTestDeliveryRuntimeShouldJoinThread(false));
}

TEST(DeliveryRuntimeAlgorithmsTest, RunsOnlyPresentLoopCallbacks)
{
    EXPECT_TRUE(MemoChatTestDeliveryRuntimeShouldRunLoop(true));
    EXPECT_FALSE(MemoChatTestDeliveryRuntimeShouldRunLoop(false));
}

#include <gtest/gtest.h>

unsigned MemoChatTestChatSessionHandlerCount();
unsigned MemoChatTestChatRelationHandlerCount();
unsigned MemoChatTestPrivateMessageHandlerCount();
unsigned MemoChatTestGroupMessageHandlerCount();
unsigned MemoChatTestAsyncEventDispatcherHandlerCount();
unsigned MemoChatTestExpectedTotalHandlerCount();
bool MemoChatTestShouldRegisterAsyncEventDispatcherHandlers();
bool MemoChatTestDidRegistrarAddExpectedHandlers(unsigned long long before_size,
                                                 unsigned long long after_size,
                                                 unsigned expected_count);

TEST(ChatHandlerRegistrarAlgorithmsTest, ExposesHandlerCountsForEachRegistrar)
{
    EXPECT_EQ(MemoChatTestChatSessionHandlerCount(), 3U);
    EXPECT_EQ(MemoChatTestChatRelationHandlerCount(), 7U);
    EXPECT_EQ(MemoChatTestPrivateMessageHandlerCount(), 6U);
    EXPECT_EQ(MemoChatTestGroupMessageHandlerCount(), 18U);
    EXPECT_EQ(MemoChatTestAsyncEventDispatcherHandlerCount(), 0U);
    EXPECT_EQ(MemoChatTestExpectedTotalHandlerCount(), 34U);
}

TEST(ChatHandlerRegistrarAlgorithmsTest, KeepsAsyncEventDispatcherRegistrationDisabledUntilHandlersExist)
{
    EXPECT_FALSE(MemoChatTestShouldRegisterAsyncEventDispatcherHandlers());
}

TEST(ChatHandlerRegistrarAlgorithmsTest, ClassifiesExpectedMapGrowth)
{
    EXPECT_TRUE(MemoChatTestDidRegistrarAddExpectedHandlers(10ULL, 13ULL, 3U));
    EXPECT_TRUE(MemoChatTestDidRegistrarAddExpectedHandlers(0ULL, 18ULL, 18U));

    EXPECT_FALSE(MemoChatTestDidRegistrarAddExpectedHandlers(10ULL, 12ULL, 3U));
    EXPECT_FALSE(MemoChatTestDidRegistrarAddExpectedHandlers(13ULL, 10ULL, 3U));
}

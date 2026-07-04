#include <gtest/gtest.h>

unsigned MemoChatTestMongoMgrEnabledForwardCount();
unsigned MemoChatTestMongoMgrPrivateMessageForwardCount();
unsigned MemoChatTestMongoMgrGroupMessageForwardCount();
unsigned MemoChatTestMongoMgrForwardingSurfaceCount();
bool MemoChatTestMongoMgrCompleteSurface(unsigned enabled_count,
                                         unsigned private_message_count,
                                         unsigned group_message_count);

TEST(MongoMgrAlgorithmsTest, KeepsForwardingSurfaceCountsStable)
{
    EXPECT_EQ(MemoChatTestMongoMgrEnabledForwardCount(), 1U);
    EXPECT_EQ(MemoChatTestMongoMgrPrivateMessageForwardCount(), 5U);
    EXPECT_EQ(MemoChatTestMongoMgrGroupMessageForwardCount(), 5U);
    EXPECT_EQ(MemoChatTestMongoMgrForwardingSurfaceCount(), 11U);
}

TEST(MongoMgrAlgorithmsTest, ClassifiesCompleteForwardingSurface)
{
    EXPECT_TRUE(MemoChatTestMongoMgrCompleteSurface(1, 5, 5));
    EXPECT_FALSE(MemoChatTestMongoMgrCompleteSurface(0, 5, 5));
    EXPECT_FALSE(MemoChatTestMongoMgrCompleteSurface(1, 4, 5));
    EXPECT_FALSE(MemoChatTestMongoMgrCompleteSurface(1, 5, 4));
}

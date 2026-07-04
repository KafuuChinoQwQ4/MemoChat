#include <gtest/gtest.h>

bool MemoChatTestChatSessionRepositoryShouldAcquireDuplicateLoginLock(int uid);
bool MemoChatTestChatSessionRepositoryShouldReleaseDuplicateLoginLock(int uid, bool lock_identifier_empty);
bool MemoChatTestChatSessionRepositoryShouldQueryUndeliveredPrivateMessages(int uid, int limit);

TEST(ChatSessionRepositoryAlgorithmsTest, GuardsInvalidUserIdsBeforeDuplicateLoginLockAcquire)
{
    EXPECT_FALSE(MemoChatTestChatSessionRepositoryShouldAcquireDuplicateLoginLock(0));
    EXPECT_FALSE(MemoChatTestChatSessionRepositoryShouldAcquireDuplicateLoginLock(-42));
    EXPECT_TRUE(MemoChatTestChatSessionRepositoryShouldAcquireDuplicateLoginLock(1));
}

TEST(ChatSessionRepositoryAlgorithmsTest, ReleasesOnlyPositiveUserIdsWithNonEmptyLockIdentifier)
{
    EXPECT_FALSE(MemoChatTestChatSessionRepositoryShouldReleaseDuplicateLoginLock(0, false));
    EXPECT_FALSE(MemoChatTestChatSessionRepositoryShouldReleaseDuplicateLoginLock(-7, false));
    EXPECT_FALSE(MemoChatTestChatSessionRepositoryShouldReleaseDuplicateLoginLock(1, true));
    EXPECT_TRUE(MemoChatTestChatSessionRepositoryShouldReleaseDuplicateLoginLock(1, false));
}

TEST(ChatSessionRepositoryAlgorithmsTest, QueriesUndeliveredMessagesOnlyForPositiveUserAndLimit)
{
    EXPECT_TRUE(MemoChatTestChatSessionRepositoryShouldQueryUndeliveredPrivateMessages(1, 20));
    EXPECT_FALSE(MemoChatTestChatSessionRepositoryShouldQueryUndeliveredPrivateMessages(0, 20));
    EXPECT_FALSE(MemoChatTestChatSessionRepositoryShouldQueryUndeliveredPrivateMessages(-1, 20));
    EXPECT_FALSE(MemoChatTestChatSessionRepositoryShouldQueryUndeliveredPrivateMessages(1, 0));
    EXPECT_FALSE(MemoChatTestChatSessionRepositoryShouldQueryUndeliveredPrivateMessages(1, -5));
}

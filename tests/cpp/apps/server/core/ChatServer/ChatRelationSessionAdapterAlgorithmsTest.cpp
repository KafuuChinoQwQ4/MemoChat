#include <gtest/gtest.h>

namespace memochat::tests::chat::relation_session_adapter
{
bool ShouldHandleCommand(bool relation_service_present);
bool ShouldSendResult(bool session_present);
unsigned SearchUserForwardCount();
unsigned FriendApplyForwardCount();
unsigned DeleteFriendForwardCount();
unsigned DialogForwardCount();
unsigned ForwardingSurfaceCount();
bool IsCompleteForwardingSurface(unsigned search_user_count,
                                 unsigned friend_apply_count,
                                 unsigned delete_friend_count,
                                 unsigned dialog_count);
} // namespace memochat::tests::chat::relation_session_adapter

namespace rsa = memochat::tests::chat::relation_session_adapter;

TEST(ChatRelationSessionAdapterAlgorithmsTest, HandlesOnlyWhenServiceWired)
{
    EXPECT_TRUE(rsa::ShouldHandleCommand(true));
    EXPECT_FALSE(rsa::ShouldHandleCommand(false));
}

TEST(ChatRelationSessionAdapterAlgorithmsTest, SendsResultOnlyWhenSessionPresent)
{
    EXPECT_TRUE(rsa::ShouldSendResult(true));
    EXPECT_FALSE(rsa::ShouldSendResult(false));
}

TEST(ChatRelationSessionAdapterAlgorithmsTest, PinsForwardingSurfaceCounts)
{
    EXPECT_EQ(rsa::SearchUserForwardCount(), 1u);
    EXPECT_EQ(rsa::FriendApplyForwardCount(), 2u);
    EXPECT_EQ(rsa::DeleteFriendForwardCount(), 1u);
    EXPECT_EQ(rsa::DialogForwardCount(), 2u);
    EXPECT_EQ(rsa::ForwardingSurfaceCount(), 6u);
}

TEST(ChatRelationSessionAdapterAlgorithmsTest, CompleteSurfaceRequiresExactCounts)
{
    EXPECT_TRUE(rsa::IsCompleteForwardingSurface(1, 2, 1, 2));
    EXPECT_FALSE(rsa::IsCompleteForwardingSurface(0, 2, 1, 2));
    EXPECT_FALSE(rsa::IsCompleteForwardingSurface(1, 1, 1, 2));
    EXPECT_FALSE(rsa::IsCompleteForwardingSurface(1, 2, 2, 2));
    EXPECT_FALSE(rsa::IsCompleteForwardingSurface(1, 2, 1, 3));
}

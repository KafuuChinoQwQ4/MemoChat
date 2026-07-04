#include <gtest/gtest.h>

bool MemoChatTestRelationRepositoryShouldQueryPrivateFriendship(int self_id, int friend_id);
bool MemoChatTestRelationRepositoryShouldFilterFriendUids(int viewer_uid, bool author_list_empty);
bool MemoChatTestRelationRepositoryShouldQueryGroupMembership(long long group_id, int uid);

TEST(RelationRepositoryAlgorithmsTest, QueriesPrivateFriendshipOnlyForDistinctPositiveUsers)
{
    EXPECT_TRUE(MemoChatTestRelationRepositoryShouldQueryPrivateFriendship(1, 2));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldQueryPrivateFriendship(0, 2));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldQueryPrivateFriendship(1, 0));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldQueryPrivateFriendship(-1, 2));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldQueryPrivateFriendship(7, 7));
}

TEST(RelationRepositoryAlgorithmsTest, SkipsFriendFilteringWithoutViewerOrAuthors)
{
    EXPECT_TRUE(MemoChatTestRelationRepositoryShouldFilterFriendUids(1, false));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldFilterFriendUids(0, false));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldFilterFriendUids(-1, false));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldFilterFriendUids(1, true));
}

TEST(RelationRepositoryAlgorithmsTest, QueriesGroupMembershipOnlyForPositiveGroupAndUser)
{
    EXPECT_TRUE(MemoChatTestRelationRepositoryShouldQueryGroupMembership(100, 1));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldQueryGroupMembership(0, 1));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldQueryGroupMembership(-1, 1));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldQueryGroupMembership(100, 0));
    EXPECT_FALSE(MemoChatTestRelationRepositoryShouldQueryGroupMembership(100, -1));
}

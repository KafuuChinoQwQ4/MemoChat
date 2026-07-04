#include <gtest/gtest.h>

namespace memochat::tests::chat::relation_service
{
const char* PrivateDialogType();
const char* GroupDialogType();
const char* RelationNotifyTaskName();
const char* FriendApplyCreatedEvent();
const char* FriendApplyApprovedEvent();
const char* FriendDeletedEvent();
int InternalReadResponseMessageId();
bool ShouldRejectSearchUserRequest(bool has_user_id, bool user_id_empty);
bool ShouldRejectSearchUserResult(bool found_user, int uid);
bool ShouldFilterFriendUids(int viewer_uid, bool has_author_uids);
bool ShouldRejectPositiveUid(int uid);
bool ShouldRejectDeleteFriend(int uid, int friend_uid);
bool ShouldRejectDialogType(bool matches_private, bool matches_group);
bool ShouldRejectPrivateDialogTarget(int peer_uid, bool is_private_friend);
bool ShouldRejectGroupDialogTarget(long long group_id, bool is_group_member);
int NormalizeMuteState(int mute_state);
} // namespace memochat::tests::chat::relation_service

TEST(ChatRelationServiceAlgorithmsTest, KeepsStableLiterals)
{
    using namespace memochat::tests::chat::relation_service;

    EXPECT_STREQ(PrivateDialogType(), "private");
    EXPECT_STREQ(GroupDialogType(), "group");
    EXPECT_STREQ(RelationNotifyTaskName(), "relation_notify");
    EXPECT_STREQ(FriendApplyCreatedEvent(), "friend_apply_created");
    EXPECT_STREQ(FriendApplyApprovedEvent(), "friend_apply_approved");
    EXPECT_STREQ(FriendDeletedEvent(), "friend_deleted");
    EXPECT_EQ(InternalReadResponseMessageId(), 0);
}

TEST(ChatRelationServiceAlgorithmsTest, ClassifiesSearchUserRequests)
{
    using namespace memochat::tests::chat::relation_service;

    EXPECT_TRUE(ShouldRejectSearchUserRequest(false, true));
    EXPECT_TRUE(ShouldRejectSearchUserRequest(false, false));
    EXPECT_TRUE(ShouldRejectSearchUserRequest(true, true));
    EXPECT_FALSE(ShouldRejectSearchUserRequest(true, false));

    EXPECT_TRUE(ShouldRejectSearchUserResult(false, 42));
    EXPECT_TRUE(ShouldRejectSearchUserResult(true, 0));
    EXPECT_TRUE(ShouldRejectSearchUserResult(true, -1));
    EXPECT_FALSE(ShouldRejectSearchUserResult(true, 42));
}

TEST(ChatRelationServiceAlgorithmsTest, ClassifiesFriendAndUidGuards)
{
    using namespace memochat::tests::chat::relation_service;

    EXPECT_FALSE(ShouldFilterFriendUids(0, true));
    EXPECT_TRUE(ShouldFilterFriendUids(42, true));
    EXPECT_FALSE(ShouldFilterFriendUids(42, false));

    EXPECT_TRUE(ShouldRejectPositiveUid(0));
    EXPECT_TRUE(ShouldRejectPositiveUid(-5));
    EXPECT_FALSE(ShouldRejectPositiveUid(7));

    EXPECT_TRUE(ShouldRejectDeleteFriend(0, 10));
    EXPECT_TRUE(ShouldRejectDeleteFriend(10, 0));
    EXPECT_TRUE(ShouldRejectDeleteFriend(10, 10));
    EXPECT_FALSE(ShouldRejectDeleteFriend(10, 11));
}

TEST(ChatRelationServiceAlgorithmsTest, ClassifiesDialogGuards)
{
    using namespace memochat::tests::chat::relation_service;

    EXPECT_FALSE(ShouldRejectDialogType(true, false));
    EXPECT_FALSE(ShouldRejectDialogType(false, true));
    EXPECT_FALSE(ShouldRejectDialogType(true, true));
    EXPECT_TRUE(ShouldRejectDialogType(false, false));

    EXPECT_TRUE(ShouldRejectPrivateDialogTarget(0, true));
    EXPECT_TRUE(ShouldRejectPrivateDialogTarget(9, false));
    EXPECT_FALSE(ShouldRejectPrivateDialogTarget(9, true));

    EXPECT_TRUE(ShouldRejectGroupDialogTarget(0, true));
    EXPECT_TRUE(ShouldRejectGroupDialogTarget(9, false));
    EXPECT_FALSE(ShouldRejectGroupDialogTarget(9, true));
}

TEST(ChatRelationServiceAlgorithmsTest, NormalizesMuteState)
{
    using namespace memochat::tests::chat::relation_service;

    EXPECT_EQ(NormalizeMuteState(-1), 0);
    EXPECT_EQ(NormalizeMuteState(0), 0);
    EXPECT_EQ(NormalizeMuteState(1), 1);
    EXPECT_EQ(NormalizeMuteState(99), 1);
}

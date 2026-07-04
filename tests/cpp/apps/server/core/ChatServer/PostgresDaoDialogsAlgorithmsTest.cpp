#include <gtest/gtest.h>

bool MemoChatTestPostgresDaoDialogsCanLoadMeta(int owner_uid);
bool MemoChatTestPostgresDaoDialogsCanReadPrivateRuntime(int owner_uid, int peer_uid);
bool MemoChatTestPostgresDaoDialogsCanReadGroupRuntime(int owner_uid, long long group_id);
bool MemoChatTestPostgresDaoDialogsKeepUid(int uid);
bool MemoChatTestPostgresDaoDialogsKeepGroup(long long group_id);
int MemoChatTestPostgresDaoDialogsConversationMin(int left_uid, int right_uid);
int MemoChatTestPostgresDaoDialogsConversationMax(int left_uid, int right_uid);
int MemoChatTestPostgresDaoDialogsNormalizeUnread(int unread_count);
bool MemoChatTestPostgresDaoDialogsCanUpsertGroupRead(int uid, long long group_id);
bool MemoChatTestPostgresDaoDialogsFallbackTimestamp(long long timestamp_ms);
bool MemoChatTestPostgresDaoDialogsKnownType(bool is_private, bool is_group);
int MemoChatTestPostgresDaoDialogsNormalizePeer(bool is_private, int peer_uid);
long long MemoChatTestPostgresDaoDialogsNormalizeGroup(bool is_group, long long group_id);
bool MemoChatTestPostgresDaoDialogsCanUpsertDialog(int owner_uid,
                                                   bool is_private,
                                                   bool is_group,
                                                   int normalized_peer_uid,
                                                   long long normalized_group_id);
int MemoChatTestPostgresDaoDialogsMaxDraftLength();
int MemoChatTestPostgresDaoDialogsNormalizePinned(int pinned_rank);
int MemoChatTestPostgresDaoDialogsNormalizeMute(int mute_state);
int MemoChatTestPostgresDaoDialogsClampApplyLimit(int limit);
bool MemoChatTestPostgresDaoDialogsGroupCodeHeader(int length, char prefix, char first_digit);
bool MemoChatTestPostgresDaoDialogsGroupCodeTail(char c);

TEST(PostgresDaoDialogsAlgorithmsTest, GatesDialogReads)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsCanLoadMeta(7));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanLoadMeta(0));

    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsCanReadPrivateRuntime(7, 8));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanReadPrivateRuntime(0, 8));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanReadPrivateRuntime(7, 0));

    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsCanReadGroupRuntime(7, 10));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanReadGroupRuntime(0, 10));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanReadGroupRuntime(7, 0));
}

TEST(PostgresDaoDialogsAlgorithmsTest, NormalizesConversationAndUnreadState)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsKeepUid(1));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsKeepUid(0));
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsKeepGroup(1));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsKeepGroup(-1));

    EXPECT_EQ(7, MemoChatTestPostgresDaoDialogsConversationMin(9, 7));
    EXPECT_EQ(9, MemoChatTestPostgresDaoDialogsConversationMax(9, 7));
    EXPECT_EQ(0, MemoChatTestPostgresDaoDialogsNormalizeUnread(-5));
    EXPECT_EQ(0, MemoChatTestPostgresDaoDialogsNormalizeUnread(0));
    EXPECT_EQ(3, MemoChatTestPostgresDaoDialogsNormalizeUnread(3));
}

TEST(PostgresDaoDialogsAlgorithmsTest, GatesGroupReadStateAndTimestampFallback)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsCanUpsertGroupRead(7, 10));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanUpsertGroupRead(0, 10));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanUpsertGroupRead(7, 0));

    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsFallbackTimestamp(0));
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsFallbackTimestamp(-1));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsFallbackTimestamp(1));
}

TEST(PostgresDaoDialogsAlgorithmsTest, NormalizesDialogTarget)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsKnownType(true, false));
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsKnownType(false, true));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsKnownType(false, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsKnownType(true, true));

    EXPECT_EQ(8, MemoChatTestPostgresDaoDialogsNormalizePeer(true, 8));
    EXPECT_EQ(0, MemoChatTestPostgresDaoDialogsNormalizePeer(false, 8));
    EXPECT_EQ(10, MemoChatTestPostgresDaoDialogsNormalizeGroup(true, 10));
    EXPECT_EQ(0, MemoChatTestPostgresDaoDialogsNormalizeGroup(false, 10));

    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsCanUpsertDialog(7, true, false, 8, 0));
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsCanUpsertDialog(7, false, true, 0, 10));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanUpsertDialog(0, true, false, 8, 0));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanUpsertDialog(7, true, false, 0, 0));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsCanUpsertDialog(7, false, true, 0, 0));
}

TEST(PostgresDaoDialogsAlgorithmsTest, NormalizesDialogFieldsAndReviewerLimit)
{
    EXPECT_EQ(2000, MemoChatTestPostgresDaoDialogsMaxDraftLength());
    EXPECT_EQ(0, MemoChatTestPostgresDaoDialogsNormalizePinned(-1));
    EXPECT_EQ(0, MemoChatTestPostgresDaoDialogsNormalizePinned(0));
    EXPECT_EQ(4, MemoChatTestPostgresDaoDialogsNormalizePinned(4));

    EXPECT_EQ(0, MemoChatTestPostgresDaoDialogsNormalizeMute(-1));
    EXPECT_EQ(0, MemoChatTestPostgresDaoDialogsNormalizeMute(0));
    EXPECT_EQ(1, MemoChatTestPostgresDaoDialogsNormalizeMute(1));
    EXPECT_EQ(1, MemoChatTestPostgresDaoDialogsNormalizeMute(99));

    EXPECT_EQ(1, MemoChatTestPostgresDaoDialogsClampApplyLimit(-10));
    EXPECT_EQ(1, MemoChatTestPostgresDaoDialogsClampApplyLimit(0));
    EXPECT_EQ(55, MemoChatTestPostgresDaoDialogsClampApplyLimit(55));
    EXPECT_EQ(100, MemoChatTestPostgresDaoDialogsClampApplyLimit(101));
}

TEST(PostgresDaoDialogsAlgorithmsTest, ValidatesGroupCodeShape)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsGroupCodeHeader(10, 'g', '1'));
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsGroupCodeHeader(10, 'g', '9'));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsGroupCodeHeader(9, 'g', '1'));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsGroupCodeHeader(10, 'u', '1'));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsGroupCodeHeader(10, 'g', '0'));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsGroupCodeHeader(10, 'g', 'x'));

    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsGroupCodeTail('0'));
    EXPECT_TRUE(MemoChatTestPostgresDaoDialogsGroupCodeTail('9'));
    EXPECT_FALSE(MemoChatTestPostgresDaoDialogsGroupCodeTail('x'));
}

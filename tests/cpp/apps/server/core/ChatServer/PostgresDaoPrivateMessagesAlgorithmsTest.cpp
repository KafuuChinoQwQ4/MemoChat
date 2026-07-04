#include <gtest/gtest.h>

long long MemoChatTestPostgresDaoPrivateMessagesRevokeWindow();
int MemoChatTestPostgresDaoPrivateMessagesConvMin(int uid, int peer_uid);
int MemoChatTestPostgresDaoPrivateMessagesConvMax(int uid, int peer_uid);
bool MemoChatTestPostgresDaoPrivateMessagesCanReadUndelivered(int to_uid, int limit);
bool MemoChatTestPostgresDaoPrivateMessagesCanReadHistory(int uid, int peer_uid, int limit);
int MemoChatTestPostgresDaoPrivateMessagesFetchLimit(int requested_limit);
bool MemoChatTestPostgresDaoPrivateMessagesTieBreaker(long long before_ts, bool before_msg_id_empty);
bool MemoChatTestPostgresDaoPrivateMessagesTimestampCursor(long long before_ts);
bool MemoChatTestPostgresDaoPrivateMessagesHasOverflow(int loaded_count, int requested_limit);
bool MemoChatTestPostgresDaoPrivateMessagesCanFind(bool msg_id_empty);
bool MemoChatTestPostgresDaoPrivateMessagesCanUpdate(int uid, int peer_uid, bool msg_id_empty, bool content_empty);
bool MemoChatTestPostgresDaoPrivateMessagesFallbackTimestamp(long long timestamp_ms);
bool MemoChatTestPostgresDaoPrivateMessagesOwner(int conv_min, int conv_max, int from_uid, int uid, int peer_uid);
bool MemoChatTestPostgresDaoPrivateMessagesCanRequestRevoke(int uid, int peer_uid, bool msg_id_empty);
bool MemoChatTestPostgresDaoPrivateMessagesCanRevoke(bool owner_matches,
                                                     long long existing_deleted_at_ms,
                                                     long long created_at_ms,
                                                     long long deleted_at_ms,
                                                     long long revoke_window_ms);
long long MemoChatTestPostgresDaoPrivateMessagesRevokeWindowStart(long long deleted_at_ms, long long revoke_window_ms);
bool MemoChatTestPostgresDaoPrivateMessagesCanUpsertReadState(int uid, int peer_uid);

TEST(PostgresDaoPrivateMessagesAlgorithmsTest, NormalizesConversationPair)
{
    EXPECT_EQ(1, MemoChatTestPostgresDaoPrivateMessagesConvMin(7, 1));
    EXPECT_EQ(7, MemoChatTestPostgresDaoPrivateMessagesConvMax(7, 1));
    EXPECT_EQ(1, MemoChatTestPostgresDaoPrivateMessagesConvMin(1, 7));
    EXPECT_EQ(7, MemoChatTestPostgresDaoPrivateMessagesConvMax(1, 7));
}

TEST(PostgresDaoPrivateMessagesAlgorithmsTest, GatesReadInputs)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesCanReadUndelivered(1, 20));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanReadUndelivered(0, 20));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanReadUndelivered(1, 0));

    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesCanReadHistory(1, 2, 20));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanReadHistory(0, 2, 20));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanReadHistory(1, -1, 20));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanReadHistory(1, 2, 0));
}

TEST(PostgresDaoPrivateMessagesAlgorithmsTest, SelectsHistoryCursorBranchAndOverflow)
{
    EXPECT_EQ(21, MemoChatTestPostgresDaoPrivateMessagesFetchLimit(20));

    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesTieBreaker(1000, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesTieBreaker(1000, true));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesTieBreaker(0, false));

    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesTimestampCursor(1000));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesTimestampCursor(0));

    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesHasOverflow(21, 20));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesHasOverflow(20, 20));
}

TEST(PostgresDaoPrivateMessagesAlgorithmsTest, GatesFindAndUpdate)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesCanFind(false));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanFind(true));

    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesCanUpdate(1, 2, false, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanUpdate(0, 2, false, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanUpdate(1, 0, false, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanUpdate(1, 2, true, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanUpdate(1, 2, false, true));

    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesFallbackTimestamp(1));
    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesFallbackTimestamp(0));
    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesFallbackTimestamp(-1));
}

TEST(PostgresDaoPrivateMessagesAlgorithmsTest, ValidatesOwnerAndRevokeWindow)
{
    const long long window_ms = MemoChatTestPostgresDaoPrivateMessagesRevokeWindow();
    EXPECT_EQ(300000, window_ms);

    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesOwner(1, 7, 7, 7, 1));
    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesOwner(1, 7, 1, 1, 7));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesOwner(1, 7, 1, 7, 1));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesOwner(1, 8, 7, 7, 1));

    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesCanRequestRevoke(1, 2, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanRequestRevoke(0, 2, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanRequestRevoke(1, 0, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanRequestRevoke(1, 2, true));

    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesCanRevoke(true, 0, 1000, 1000 + window_ms, window_ms));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanRevoke(false, 0, 1000, 1000 + window_ms, window_ms));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanRevoke(true, 1, 1000, 1000 + window_ms, window_ms));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanRevoke(true, 0, 0, 1000 + window_ms, window_ms));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanRevoke(true, 0, 1000, 1001 + window_ms, window_ms));

    EXPECT_EQ(7000, MemoChatTestPostgresDaoPrivateMessagesRevokeWindowStart(10000, 3000));
}

TEST(PostgresDaoPrivateMessagesAlgorithmsTest, GatesPrivateReadState)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoPrivateMessagesCanUpsertReadState(1, 2));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanUpsertReadState(0, 2));
    EXPECT_FALSE(MemoChatTestPostgresDaoPrivateMessagesCanUpsertReadState(1, 0));
}

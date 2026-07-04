#include <gtest/gtest.h>

bool MemoChatTestMongoDaoParseBoolText(const char* text);
bool MemoChatTestMongoDaoEnabled(bool enabled, bool init_ok, bool has_pool);
bool MemoChatTestMongoDaoShouldInitialize(bool enabled);
bool MemoChatTestMongoDaoHasRequiredConfig(bool uri_empty, bool database_empty);
bool MemoChatTestMongoDaoUseDefaultCollection(bool collection_name_empty);
int MemoChatTestMongoDaoClampHistoryLimit(int limit);
bool MemoChatTestMongoDaoHasMorePage(int loaded_count, int final_limit);
bool MemoChatTestMongoDaoCanReadPrivateHistory(bool enabled, int uid, int peer_uid, int limit);
bool MemoChatTestMongoDaoPrivateTieBreaker(long long before_ts, bool before_msg_id_empty);
bool MemoChatTestMongoDaoTimestampFilter(long long value);
bool MemoChatTestMongoDaoCanUpdatePrivate(bool enabled, int uid, int peer_uid, bool msg_id_empty, bool content_empty);
bool MemoChatTestMongoDaoCanRequestPrivateRevoke(bool enabled, int uid, int peer_uid, bool msg_id_empty);
long long MemoChatTestMongoDaoSelectTimestamp(long long requested_timestamp, long long fallback_now_ms);
bool MemoChatTestMongoDaoPrivateOwner(int conv_min, int conv_max, int from_uid, int uid, int peer_uid);
bool MemoChatTestMongoDaoCanRevokePrivate(bool owner_matches,
                                          long long existing_deleted_at_ms,
                                          long long created_at_ms,
                                          long long deleted_at_ms,
                                          long long revoke_window_ms);
bool MemoChatTestMongoDaoCanSaveGroup(bool enabled, bool msg_id_empty, long long group_id);
bool MemoChatTestMongoDaoCanReadGroupHistory(bool enabled, long long group_id, int limit);
bool MemoChatTestMongoDaoGroupSeqFilter(long long before_seq);
bool MemoChatTestMongoDaoGroupTimestampFilter(long long before_seq, long long before_ts);
bool MemoChatTestMongoDaoCanRequestGroupRevoke(bool enabled, long long group_id, int operator_uid, bool msg_id_empty);
bool MemoChatTestMongoDaoCanApplyGroupRevoke(int message_from_uid,
                                             int operator_uid,
                                             long long existing_deleted_at_ms,
                                             long long created_at_ms,
                                             long long deleted_at_ms,
                                             long long revoke_window_ms);

TEST(MongoDaoAlgorithmsTest, ParsesOnlyTruthyMongoConfigText)
{
    EXPECT_TRUE(MemoChatTestMongoDaoParseBoolText("1"));
    EXPECT_TRUE(MemoChatTestMongoDaoParseBoolText("TRUE"));
    EXPECT_TRUE(MemoChatTestMongoDaoParseBoolText("Yes"));
    EXPECT_TRUE(MemoChatTestMongoDaoParseBoolText("on"));

    EXPECT_FALSE(MemoChatTestMongoDaoParseBoolText(nullptr));
    EXPECT_FALSE(MemoChatTestMongoDaoParseBoolText(""));
    EXPECT_FALSE(MemoChatTestMongoDaoParseBoolText(" true"));
    EXPECT_FALSE(MemoChatTestMongoDaoParseBoolText("false"));
}

TEST(MongoDaoAlgorithmsTest, GatesInitializationAndRequiredConfig)
{
    EXPECT_TRUE(MemoChatTestMongoDaoShouldInitialize(true));
    EXPECT_FALSE(MemoChatTestMongoDaoShouldInitialize(false));

    EXPECT_TRUE(MemoChatTestMongoDaoEnabled(true, true, true));
    EXPECT_FALSE(MemoChatTestMongoDaoEnabled(false, true, true));
    EXPECT_FALSE(MemoChatTestMongoDaoEnabled(true, false, true));
    EXPECT_FALSE(MemoChatTestMongoDaoEnabled(true, true, false));

    EXPECT_TRUE(MemoChatTestMongoDaoHasRequiredConfig(false, false));
    EXPECT_FALSE(MemoChatTestMongoDaoHasRequiredConfig(true, false));
    EXPECT_FALSE(MemoChatTestMongoDaoHasRequiredConfig(false, true));
    EXPECT_TRUE(MemoChatTestMongoDaoUseDefaultCollection(true));
    EXPECT_FALSE(MemoChatTestMongoDaoUseDefaultCollection(false));
}

TEST(MongoDaoAlgorithmsTest, ClampsHistoryLimitAndDetectsOverflowPage)
{
    EXPECT_EQ(1, MemoChatTestMongoDaoClampHistoryLimit(-10));
    EXPECT_EQ(1, MemoChatTestMongoDaoClampHistoryLimit(0));
    EXPECT_EQ(17, MemoChatTestMongoDaoClampHistoryLimit(17));
    EXPECT_EQ(50, MemoChatTestMongoDaoClampHistoryLimit(51));

    EXPECT_TRUE(MemoChatTestMongoDaoHasMorePage(51, 50));
    EXPECT_FALSE(MemoChatTestMongoDaoHasMorePage(50, 50));
}

TEST(MongoDaoAlgorithmsTest, GatesPrivateHistoryAndCursorFilters)
{
    EXPECT_TRUE(MemoChatTestMongoDaoCanReadPrivateHistory(true, 1, 2, 20));
    EXPECT_FALSE(MemoChatTestMongoDaoCanReadPrivateHistory(false, 1, 2, 20));
    EXPECT_FALSE(MemoChatTestMongoDaoCanReadPrivateHistory(true, 0, 2, 20));
    EXPECT_FALSE(MemoChatTestMongoDaoCanReadPrivateHistory(true, 1, -1, 20));
    EXPECT_FALSE(MemoChatTestMongoDaoCanReadPrivateHistory(true, 1, 2, 0));

    EXPECT_TRUE(MemoChatTestMongoDaoPrivateTieBreaker(1000, false));
    EXPECT_FALSE(MemoChatTestMongoDaoPrivateTieBreaker(1000, true));
    EXPECT_FALSE(MemoChatTestMongoDaoPrivateTieBreaker(0, false));
    EXPECT_TRUE(MemoChatTestMongoDaoTimestampFilter(1000));
    EXPECT_FALSE(MemoChatTestMongoDaoTimestampFilter(0));
}

TEST(MongoDaoAlgorithmsTest, GatesPrivateEditAndTimestampSelection)
{
    EXPECT_TRUE(MemoChatTestMongoDaoCanUpdatePrivate(true, 1, 2, false, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanUpdatePrivate(false, 1, 2, false, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanUpdatePrivate(true, 0, 2, false, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanUpdatePrivate(true, 1, 0, false, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanUpdatePrivate(true, 1, 2, true, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanUpdatePrivate(true, 1, 2, false, true));

    EXPECT_TRUE(MemoChatTestMongoDaoCanRequestPrivateRevoke(true, 1, 2, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRequestPrivateRevoke(false, 1, 2, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRequestPrivateRevoke(true, 0, 2, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRequestPrivateRevoke(true, 1, 0, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRequestPrivateRevoke(true, 1, 2, true));

    EXPECT_EQ(1234, MemoChatTestMongoDaoSelectTimestamp(1234, 9999));
    EXPECT_EQ(9999, MemoChatTestMongoDaoSelectTimestamp(0, 9999));
    EXPECT_EQ(9999, MemoChatTestMongoDaoSelectTimestamp(-7, 9999));
}

TEST(MongoDaoAlgorithmsTest, ValidatesPrivateOwnerAndRevokeWindow)
{
    constexpr long long kWindow = 300000;

    EXPECT_TRUE(MemoChatTestMongoDaoPrivateOwner(1, 3, 3, 3, 1));
    EXPECT_TRUE(MemoChatTestMongoDaoPrivateOwner(1, 3, 1, 1, 3));
    EXPECT_FALSE(MemoChatTestMongoDaoPrivateOwner(1, 3, 2, 3, 1));
    EXPECT_FALSE(MemoChatTestMongoDaoPrivateOwner(1, 4, 3, 3, 1));

    EXPECT_TRUE(MemoChatTestMongoDaoCanRevokePrivate(true, 0, 1000, 1000 + kWindow, kWindow));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRevokePrivate(false, 0, 1000, 1000 + kWindow, kWindow));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRevokePrivate(true, 1, 1000, 1000 + kWindow, kWindow));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRevokePrivate(true, 0, 0, 1000 + kWindow, kWindow));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRevokePrivate(true, 0, 1000, 1001 + kWindow, kWindow));
}

TEST(MongoDaoAlgorithmsTest, GatesGroupHistoryAndCursorFilters)
{
    EXPECT_TRUE(MemoChatTestMongoDaoCanSaveGroup(true, false, 10));
    EXPECT_FALSE(MemoChatTestMongoDaoCanSaveGroup(false, false, 10));
    EXPECT_FALSE(MemoChatTestMongoDaoCanSaveGroup(true, true, 10));
    EXPECT_FALSE(MemoChatTestMongoDaoCanSaveGroup(true, false, 0));

    EXPECT_TRUE(MemoChatTestMongoDaoCanReadGroupHistory(true, 10, 20));
    EXPECT_FALSE(MemoChatTestMongoDaoCanReadGroupHistory(true, 0, 20));
    EXPECT_FALSE(MemoChatTestMongoDaoCanReadGroupHistory(true, 10, 0));

    EXPECT_TRUE(MemoChatTestMongoDaoGroupSeqFilter(7));
    EXPECT_FALSE(MemoChatTestMongoDaoGroupSeqFilter(0));
    EXPECT_TRUE(MemoChatTestMongoDaoGroupTimestampFilter(0, 1000));
    EXPECT_FALSE(MemoChatTestMongoDaoGroupTimestampFilter(7, 1000));
    EXPECT_FALSE(MemoChatTestMongoDaoGroupTimestampFilter(0, 0));
}

TEST(MongoDaoAlgorithmsTest, GatesGroupRevokeRequestAndLoadedMessage)
{
    constexpr long long kWindow = 300000;

    EXPECT_TRUE(MemoChatTestMongoDaoCanRequestGroupRevoke(true, 10, 3, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRequestGroupRevoke(false, 10, 3, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRequestGroupRevoke(true, 0, 3, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRequestGroupRevoke(true, 10, 0, false));
    EXPECT_FALSE(MemoChatTestMongoDaoCanRequestGroupRevoke(true, 10, 3, true));

    EXPECT_TRUE(MemoChatTestMongoDaoCanApplyGroupRevoke(3, 3, 0, 1000, 1000 + kWindow, kWindow));
    EXPECT_FALSE(MemoChatTestMongoDaoCanApplyGroupRevoke(4, 3, 0, 1000, 1000 + kWindow, kWindow));
    EXPECT_FALSE(MemoChatTestMongoDaoCanApplyGroupRevoke(3, 3, 1, 1000, 1000 + kWindow, kWindow));
    EXPECT_FALSE(MemoChatTestMongoDaoCanApplyGroupRevoke(3, 3, 0, 0, 1000 + kWindow, kWindow));
    EXPECT_FALSE(MemoChatTestMongoDaoCanApplyGroupRevoke(3, 3, 0, 1000, 1001 + kWindow, kWindow));
}

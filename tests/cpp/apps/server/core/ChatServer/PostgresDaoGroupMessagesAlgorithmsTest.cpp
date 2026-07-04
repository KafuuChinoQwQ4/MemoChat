#include <gtest/gtest.h>

long long MemoChatTestPostgresDaoGroupMessagesPermissionBit();
long long MemoChatTestPostgresDaoGroupMessagesRevokeWindow();
bool MemoChatTestPostgresDaoGroupMessagesCanSave(bool msg_id_empty, long long group_id, int from_uid);
bool MemoChatTestPostgresDaoGroupMessagesShouldLoadSeq(long long assigned_group_seq);
long long MemoChatTestPostgresDaoGroupMessagesNormalizeSeq(long long candidate_group_seq);
bool MemoChatTestPostgresDaoGroupMessagesWriteExt(bool file_name_empty, bool mime_empty, int size);
bool MemoChatTestPostgresDaoGroupMessagesCanUpdate(long long group_id,
                                                   int operator_uid,
                                                   bool msg_id_empty,
                                                   bool content_empty);
bool MemoChatTestPostgresDaoGroupMessagesFallbackTimestamp(long long timestamp_ms);
bool MemoChatTestPostgresDaoGroupMessagesHasDeletePermission(long long permissions);
bool MemoChatTestPostgresDaoGroupMessagesCanEdit(int from_uid, int operator_uid, bool has_delete_permission);
bool MemoChatTestPostgresDaoGroupMessagesCanRequestRevoke(long long group_id, int operator_uid, bool msg_id_empty);
bool MemoChatTestPostgresDaoGroupMessagesCanRevoke(int from_uid,
                                                   int operator_uid,
                                                   long long existing_deleted_at_ms,
                                                   long long created_at,
                                                   long long deleted_at_ms,
                                                   long long revoke_window_ms);
long long MemoChatTestPostgresDaoGroupMessagesRevokeWindowStart(long long deleted_at_ms, long long revoke_window_ms);
int MemoChatTestPostgresDaoGroupMessagesClampLimit(int limit);
int MemoChatTestPostgresDaoGroupMessagesFetchLimit(int final_limit);
bool MemoChatTestPostgresDaoGroupMessagesSeqCursor(long long before_seq);
bool MemoChatTestPostgresDaoGroupMessagesTimestampCursor(long long before_seq, long long before_ts);
bool MemoChatTestPostgresDaoGroupMessagesOverflow(int loaded_count, int final_limit);
bool MemoChatTestPostgresDaoGroupMessagesCanFind(long long group_id, bool msg_id_empty);

TEST(PostgresDaoGroupMessagesAlgorithmsTest, GatesSaveAndSequenceFallback)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesCanSave(false, 10, 7));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanSave(true, 10, 7));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanSave(false, 0, 7));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanSave(false, 10, 0));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesShouldLoadSeq(0));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesShouldLoadSeq(-1));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesShouldLoadSeq(3));
    EXPECT_EQ(1, MemoChatTestPostgresDaoGroupMessagesNormalizeSeq(0));
    EXPECT_EQ(1, MemoChatTestPostgresDaoGroupMessagesNormalizeSeq(-2));
    EXPECT_EQ(4, MemoChatTestPostgresDaoGroupMessagesNormalizeSeq(4));
}

TEST(PostgresDaoGroupMessagesAlgorithmsTest, GatesAttachmentExtensionWrites)
{
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesWriteExt(true, true, 0));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesWriteExt(false, true, 0));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesWriteExt(true, false, 0));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesWriteExt(true, true, 1));
}

TEST(PostgresDaoGroupMessagesAlgorithmsTest, GatesEditInputsAndPermission)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesCanUpdate(10, 7, false, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanUpdate(0, 7, false, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanUpdate(10, 0, false, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanUpdate(10, 7, true, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanUpdate(10, 7, false, true));

    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesFallbackTimestamp(1));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesFallbackTimestamp(0));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesFallbackTimestamp(-1));

    const long long permission_bit = MemoChatTestPostgresDaoGroupMessagesPermissionBit();
    EXPECT_EQ(2, permission_bit);
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesHasDeletePermission(permission_bit));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesHasDeletePermission(permission_bit | 8));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesHasDeletePermission(8));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesCanEdit(7, 7, false));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesCanEdit(7, 8, true));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanEdit(7, 8, false));
}

TEST(PostgresDaoGroupMessagesAlgorithmsTest, GatesRevokeRequestAndLoadedMessage)
{
    const long long window_ms = MemoChatTestPostgresDaoGroupMessagesRevokeWindow();
    EXPECT_EQ(300000, window_ms);

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesCanRequestRevoke(10, 7, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanRequestRevoke(0, 7, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanRequestRevoke(10, 0, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanRequestRevoke(10, 7, true));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesCanRevoke(7, 7, 0, 1000, 1000 + window_ms, window_ms));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanRevoke(7, 8, 0, 1000, 1000 + window_ms, window_ms));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanRevoke(7, 7, 1, 1000, 1000 + window_ms, window_ms));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanRevoke(7, 7, 0, 0, 1000 + window_ms, window_ms));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanRevoke(7, 7, 0, 1000, 1001 + window_ms, window_ms));
    EXPECT_EQ(7000, MemoChatTestPostgresDaoGroupMessagesRevokeWindowStart(10000, 3000));
}

TEST(PostgresDaoGroupMessagesAlgorithmsTest, ClampsHistoryLimitAndSelectsCursor)
{
    EXPECT_EQ(1, MemoChatTestPostgresDaoGroupMessagesClampLimit(-10));
    EXPECT_EQ(1, MemoChatTestPostgresDaoGroupMessagesClampLimit(0));
    EXPECT_EQ(17, MemoChatTestPostgresDaoGroupMessagesClampLimit(17));
    EXPECT_EQ(50, MemoChatTestPostgresDaoGroupMessagesClampLimit(51));
    EXPECT_EQ(18, MemoChatTestPostgresDaoGroupMessagesFetchLimit(17));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesSeqCursor(7));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesSeqCursor(0));
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesTimestampCursor(0, 1000));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesTimestampCursor(7, 1000));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesTimestampCursor(0, 0));

    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesOverflow(51, 50));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesOverflow(50, 50));
}

TEST(PostgresDaoGroupMessagesAlgorithmsTest, GatesFindByMessageId)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoGroupMessagesCanFind(10, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanFind(0, false));
    EXPECT_FALSE(MemoChatTestPostgresDaoGroupMessagesCanFind(10, true));
}

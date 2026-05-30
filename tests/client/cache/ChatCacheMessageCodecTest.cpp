#include "ChatCacheMessageCodec.h"

#include <gtest/gtest.h>

QString gate_url_prefix;
QString gate_media_url_prefix;

TEST(ChatCacheMessageCodecTest, NormalizesRequiredAndOptionalTimestamps)
{
    EXPECT_EQ(normalizeChatCacheTimestamp(1234567890), static_cast<qint64>(1234567890000));
    EXPECT_EQ(normalizeChatCacheTimestamp(1234567890000), static_cast<qint64>(1234567890000));
    EXPECT_GT(normalizeChatCacheTimestamp(0), static_cast<qint64>(0));

    EXPECT_EQ(normalizeChatCacheOptionalTimestamp(0), static_cast<qint64>(0));
    EXPECT_EQ(normalizeChatCacheOptionalTimestamp(-1), static_cast<qint64>(0));
    EXPECT_EQ(normalizeChatCacheOptionalTimestamp(1234567890), static_cast<qint64>(1234567890000));
}

TEST(ChatCacheMessageCodecTest, DeletedStateWinsOverEditedState)
{
    EXPECT_EQ(cacheMessageState(0, 0), QStringLiteral("sent"));
    EXPECT_EQ(cacheMessageState(1234567890, 0), QStringLiteral("edited"));
    EXPECT_EQ(cacheMessageState(1234567890, 1234567891), QStringLiteral("deleted"));
}

TEST(ChatCacheMessageCodecTest, BuildsPrivateMessageFromCacheRow)
{
    PrivateChatCacheRow row;
    row.msgId = QStringLiteral("m1");
    row.content = QStringLiteral("hello");
    row.fromUid = 10;
    row.toUid = 20;
    row.createdAt = 1234567890;
    row.replyToServerMsgId = 88;
    row.forwardMetaJson = QStringLiteral("{\"kind\":\"forward\"}");
    row.editedAtMs = 1234567900;

    auto message = privateMessageFromCacheRow(row);

    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->_msg_id, QStringLiteral("m1"));
    EXPECT_EQ(message->_msg_content, QStringLiteral("hello"));
    EXPECT_EQ(message->_from_uid, 10);
    EXPECT_EQ(message->_to_uid, 20);
    EXPECT_EQ(message->_created_at, static_cast<qint64>(1234567890000));
    EXPECT_EQ(message->_msg_state, QStringLiteral("edited"));
    EXPECT_EQ(message->_reply_to_server_msg_id, 88);
    EXPECT_EQ(message->_forward_meta_json, QStringLiteral("{\"kind\":\"forward\"}"));
    EXPECT_EQ(message->_edited_at_ms, static_cast<qint64>(1234567900000));
}

TEST(ChatCacheMessageCodecTest, BuildsGroupMessageFromCacheRow)
{
    GroupChatCacheRow row;
    row.msgId = QStringLiteral("g1");
    row.content = QStringLiteral("group hello");
    row.fromUid = 10;
    row.fromName = QStringLiteral("Alice");
    row.fromIcon = QStringLiteral("qrc:/res/head_1.png");
    row.createdAt = 1234567890000;
    row.serverMsgId = 99;
    row.groupSeq = 7;
    row.replyToServerMsgId = 12;
    row.forwardMetaJson = QStringLiteral("{\"kind\":\"group-forward\"}");
    row.deletedAtMs = 1234567999;

    auto message = groupMessageFromCacheRow(row);

    ASSERT_NE(message, nullptr);
    EXPECT_EQ(message->_msg_id, QStringLiteral("g1"));
    EXPECT_EQ(message->_msg_content, QStringLiteral("group hello"));
    EXPECT_EQ(message->_from_uid, 10);
    EXPECT_EQ(message->_to_uid, 0);
    EXPECT_EQ(message->_from_name, QStringLiteral("Alice"));
    EXPECT_EQ(message->_from_icon, QStringLiteral("qrc:/res/head_1.png"));
    EXPECT_EQ(message->_server_msg_id, 99);
    EXPECT_EQ(message->_group_seq, 7);
    EXPECT_EQ(message->_reply_to_server_msg_id, 12);
    EXPECT_EQ(message->_msg_state, QStringLiteral("deleted"));
    EXPECT_EQ(message->_deleted_at_ms, static_cast<qint64>(1234567999000));
}

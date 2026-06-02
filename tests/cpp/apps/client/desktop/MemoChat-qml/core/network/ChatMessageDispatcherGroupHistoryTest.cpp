#include "ChatMessageDispatcherGroupHistory.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>

TEST(ChatMessageDispatcherGroupHistoryTest, BuildHistoryTextMessageMapsAllFields)
{
    QJsonObject message;
    message["msgid"] = QStringLiteral("group-msg-1");
    message["content"] = QStringLiteral("hello group");
    message["fromuid"] = 123;
    message["from_nick"] = QStringLiteral("Nick");
    message["from_icon"] = QStringLiteral("qrc:/avatar.png");
    message["created_at"] = 1700000000LL;
    message["server_msg_id"] = 42LL;
    message["group_seq"] = 7LL;
    message["reply_to_server_msg_id"] = 5LL;
    message["forward_meta"] = QJsonObject{{"source", "group"}, {"count", 2}};
    message["edited_at_ms"] = 1700000000123LL;
    message["deleted_at_ms"] = 0;

    const auto built = ChatMessageDispatcherGroupHistory::buildHistoryTextMessage(message);

    ASSERT_NE(built, nullptr);
    EXPECT_EQ(built->_msg_id, QStringLiteral("group-msg-1"));
    EXPECT_EQ(built->_msg_content, QStringLiteral("hello group"));
    EXPECT_EQ(built->_from_uid, 123);
    EXPECT_EQ(built->_to_uid, 0);
    EXPECT_EQ(built->_from_name, QStringLiteral("Nick"));
    EXPECT_EQ(built->_created_at, 1700000000000LL);
    EXPECT_EQ(built->_from_icon, QStringLiteral("qrc:/avatar.png"));
    EXPECT_EQ(built->_msg_state, QStringLiteral("edited"));
    EXPECT_EQ(built->_server_msg_id, 42LL);
    EXPECT_EQ(built->_group_seq, 7LL);
    EXPECT_EQ(built->_reply_to_server_msg_id, 5LL);
    EXPECT_TRUE(built->_forward_meta_json.contains(QStringLiteral("\"source\":\"group\"")));
    EXPECT_TRUE(built->_forward_meta_json.contains(QStringLiteral("\"count\":2")));
    EXPECT_EQ(built->_edited_at_ms, 1700000000123LL);
    EXPECT_EQ(built->_deleted_at_ms, 0);
}

TEST(ChatMessageDispatcherGroupHistoryTest, BuildHistoryTextMessageFallsBackToFromName)
{
    const QJsonObject message{{"msgid", "m2"}, {"content", "fallback"}, {"fromuid", 321}, {"from_name", "Display"}};

    const auto built = ChatMessageDispatcherGroupHistory::buildHistoryTextMessage(message);

    ASSERT_NE(built, nullptr);
    EXPECT_EQ(built->_from_name, QStringLiteral("Display"));
}

TEST(ChatMessageDispatcherGroupHistoryTest, BuildHistoryTextMessageUsesPayloadStateRules)
{
    const auto edited =
        ChatMessageDispatcherGroupHistory::buildHistoryTextMessage(QJsonObject{{"msgid", "m3"}, {"edited_at_ms", 1}});
    const auto revoked =
        ChatMessageDispatcherGroupHistory::buildHistoryTextMessage(QJsonObject{{"msgid", "m4"}, {"msgtype", "revoke"}});
    const auto deleted = ChatMessageDispatcherGroupHistory::buildHistoryTextMessage(
        QJsonObject{{"msgid", "m5"}, {"content", QStringLiteral("[消息已撤回]")}});

    ASSERT_NE(edited, nullptr);
    ASSERT_NE(revoked, nullptr);
    ASSERT_NE(deleted, nullptr);
    EXPECT_EQ(edited->_msg_state, QStringLiteral("edited"));
    EXPECT_EQ(revoked->_msg_state, QStringLiteral("deleted"));
    EXPECT_EQ(deleted->_msg_state, QStringLiteral("deleted"));
}

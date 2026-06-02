#include "ChatMessageDispatcherGroupPayload.h"

#include <gtest/gtest.h>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

TEST(ChatMessageDispatcherGroupPayloadTest, NormalizeCreatedAtConvertsSecondsAndKeepsMilliseconds)
{
    EXPECT_EQ(ChatMessageDispatcherGroupPayload::normalizeCreatedAt(1700000000LL), 1700000000000LL);
    EXPECT_EQ(ChatMessageDispatcherGroupPayload::normalizeCreatedAt(1700000000000LL), 1700000000000LL);

    const qint64 before = QDateTime::currentMSecsSinceEpoch();
    const qint64 fallback = ChatMessageDispatcherGroupPayload::normalizeCreatedAt(0);
    const qint64 after = QDateTime::currentMSecsSinceEpoch();
    EXPECT_GE(fallback, before);
    EXPECT_LE(fallback, after);
}

TEST(ChatMessageDispatcherGroupPayloadTest, CompactJsonValueHandlesObjectArrayAndString)
{
    const QString objectJson =
        ChatMessageDispatcherGroupPayload::compactJsonValue(QJsonObject{{"kind", "group"}, {"count", 2}});
    const QJsonObject parsedObject = QJsonDocument::fromJson(objectJson.toUtf8()).object();
    EXPECT_EQ(parsedObject.value("kind").toString(), QStringLiteral("group"));
    EXPECT_EQ(parsedObject.value("count").toInt(), 2);
    EXPECT_FALSE(objectJson.contains(QLatin1Char(' ')));

    const QString arrayJson =
        ChatMessageDispatcherGroupPayload::compactJsonValue(QJsonArray{QStringLiteral("one"), QStringLiteral("two")});
    const QJsonArray parsedArray = QJsonDocument::fromJson(arrayJson.toUtf8()).array();
    ASSERT_EQ(parsedArray.size(), 2);
    EXPECT_EQ(parsedArray.at(0).toString(), QStringLiteral("one"));
    EXPECT_EQ(parsedArray.at(1).toString(), QStringLiteral("two"));

    EXPECT_EQ(ChatMessageDispatcherGroupPayload::compactJsonValue(QJsonValue(QStringLiteral("raw-json"))),
                                                                  QStringLiteral("raw-json"));
    EXPECT_TRUE(ChatMessageDispatcherGroupPayload::compactJsonValue(QJsonValue()).isEmpty());
}

TEST(ChatMessageDispatcherGroupPayloadTest, MessageStateDetectsDeletedEditedAndSent)
{
    EXPECT_EQ(ChatMessageDispatcherGroupPayload::messageState(QJsonObject{{"content", "hello"}, {"msgtype", "text"}}),
              QStringLiteral("sent"));
    EXPECT_EQ(ChatMessageDispatcherGroupPayload::messageState(QJsonObject{{"edited_at_ms", 1700000000000LL}}),
              QStringLiteral("edited"));
    EXPECT_EQ(ChatMessageDispatcherGroupPayload::messageState(QJsonObject{{"edited_at_ms", 1}, {"deleted_at_ms", 2}}),
              QStringLiteral("deleted"));
    EXPECT_EQ(ChatMessageDispatcherGroupPayload::messageState(QJsonObject{{"msgtype", "revoke"}}),
              QStringLiteral("deleted"));
    EXPECT_EQ(ChatMessageDispatcherGroupPayload::messageState(QJsonObject{{"content", QStringLiteral("[消息已撤回]")}}),
                                                              QStringLiteral("deleted"));
}

TEST(ChatMessageDispatcherGroupPayloadTest, NormalizedNotifyMessageMergesTopLevelFallbacks)
{
    QJsonObject payload;
    payload["created_at"] = 1700000000LL;
    payload["server_msg_id"] = 42LL;
    payload["group_seq"] = 7LL;
    payload["reply_to_server_msg_id"] = 5LL;
    payload["forward_meta"] = QJsonObject{{"source", "group"}};
    payload["edited_at_ms"] = 1700000000123LL;
    payload["deleted_at_ms"] = 0;
    payload["msg"] = QJsonObject{{"msgid", "m1"}, {"content", "hello"}};

    const QJsonObject normalized = ChatMessageDispatcherGroupPayload::normalizedNotifyMessage(payload);

    EXPECT_EQ(normalized.value("created_at").toVariant().toLongLong(), 1700000000000LL);
    EXPECT_EQ(normalized.value("server_msg_id").toVariant().toLongLong(), 42LL);
    EXPECT_EQ(normalized.value("group_seq").toVariant().toLongLong(), 7LL);
    EXPECT_EQ(normalized.value("reply_to_server_msg_id").toVariant().toLongLong(), 5LL);
    EXPECT_EQ(normalized.value("forward_meta").toObject().value("source").toString(), QStringLiteral("group"));
    EXPECT_EQ(normalized.value("edited_at_ms").toVariant().toLongLong(), 1700000000123LL);
    EXPECT_TRUE(normalized.contains("deleted_at_ms"));
}

TEST(ChatMessageDispatcherGroupPayloadTest, NormalizedNotifyMessagePreservesExistingPositiveMessageFields)
{
    QJsonObject child;
    child["created_at"] = 1700000000456LL;
    child["server_msg_id"] = 99LL;
    child["group_seq"] = 100LL;
    child["reply_to_server_msg_id"] = 101LL;
    child["forward_meta"] = QStringLiteral("child-meta");
    child["edited_at_ms"] = 88LL;

    QJsonObject payload;
    payload["created_at"] = 1700000000LL;
    payload["server_msg_id"] = 42LL;
    payload["group_seq"] = 7LL;
    payload["reply_to_server_msg_id"] = 5LL;
    payload["forward_meta"] = QJsonObject{{"source", "top"}};
    payload["edited_at_ms"] = 77LL;
    payload["deleted_at_ms"] = 66LL;
    payload["msg"] = child;

    const QJsonObject normalized = ChatMessageDispatcherGroupPayload::normalizedNotifyMessage(payload);

    EXPECT_EQ(normalized.value("created_at").toVariant().toLongLong(), 1700000000456LL);
    EXPECT_EQ(normalized.value("server_msg_id").toVariant().toLongLong(), 99LL);
    EXPECT_EQ(normalized.value("group_seq").toVariant().toLongLong(), 100LL);
    EXPECT_EQ(normalized.value("reply_to_server_msg_id").toVariant().toLongLong(), 101LL);
    EXPECT_EQ(normalized.value("forward_meta").toString(), QStringLiteral("child-meta"));
    EXPECT_EQ(normalized.value("edited_at_ms").toVariant().toLongLong(), 88LL);
    EXPECT_EQ(normalized.value("deleted_at_ms").toVariant().toLongLong(), 66LL);
}

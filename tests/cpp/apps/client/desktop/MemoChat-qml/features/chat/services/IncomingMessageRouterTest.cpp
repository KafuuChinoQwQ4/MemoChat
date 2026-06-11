#include "IncomingMessageRouter.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QStringList>

namespace
{
IncomingMessageRouterReadiness notReady()
{
    IncomingMessageRouterReadiness readiness;
    readiness.onChatPage = true;
    readiness.dialogsReady = true;
    readiness.chatListInitialized = false;
    return readiness;
}

IncomingMessageRouterReadiness ready()
{
    IncomingMessageRouterReadiness readiness;
    readiness.onChatPage = true;
    readiness.dialogsReady = true;
    readiness.chatListInitialized = true;
    return readiness;
}

std::shared_ptr<TextChatMsg> privateMessage(int fromUid, int toUid, const QString& msgId, qint64 createdAt)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("msgid"), msgId);
    obj.insert(QStringLiteral("content"), QStringLiteral("private-%1").arg(msgId));
    obj.insert(QStringLiteral("created_at"), static_cast<qint64>(createdAt));
    QJsonArray array;
    array.append(obj);
    return std::make_shared<TextChatMsg>(fromUid, toUid, array);
}

std::shared_ptr<GroupChatMsg> groupMessage(qint64 groupId, const QString& msgId, qint64 createdAt)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("msgid"), msgId);
    obj.insert(QStringLiteral("content"), QStringLiteral("group-%1").arg(msgId));
    obj.insert(QStringLiteral("created_at"), static_cast<qint64>(createdAt));
    return std::make_shared<GroupChatMsg>(groupId, 2002, obj, QStringLiteral("Alice"), QStringLiteral("a.png"));
}
} // namespace

TEST(IncomingMessageRouterTest, BuffersOnlyDuringChatBootstrap)
{
    IncomingMessageRouter router;

    EXPECT_FALSE(router.bufferPrivateMessage(ready(), privateMessage(1, 2, QStringLiteral("p1"), 1000)));
    EXPECT_EQ(router.pendingCount(), 0);

    EXPECT_TRUE(router.bufferPrivateMessage(notReady(), privateMessage(1, 2, QStringLiteral("p1"), 1000)));
    EXPECT_EQ(router.pendingCount(), 1);
}

TEST(IncomingMessageRouterTest, RoutesReadyMessagesImmediately)
{
    IncomingMessageRouter router;
    QStringList applied;
    IncomingMessageRouterDispatch dispatch;
    dispatch.applyPrivateMessage = [&applied](std::shared_ptr<TextChatMsg> msg)
    {
        ASSERT_TRUE(msg);
        ASSERT_FALSE(msg->_chat_msgs.empty());
        applied << msg->_chat_msgs.front()->_msg_id;
    };
    dispatch.applyGroupMessage = [&applied](std::shared_ptr<GroupChatMsg> msg)
    {
        ASSERT_TRUE(msg);
        ASSERT_TRUE(msg->_msg);
        applied << msg->_msg->_msg_id;
    };

    const auto privateResult =
        router.routePrivateMessage(ready(), privateMessage(1, 2, QStringLiteral("p-ready"), 1000), dispatch);
    const auto groupResult =
        router.routeGroupMessage(ready(), groupMessage(7001, QStringLiteral("g-ready"), 1001), dispatch);

    EXPECT_TRUE(privateResult.accepted);
    EXPECT_TRUE(privateResult.applied);
    EXPECT_FALSE(privateResult.buffered);
    EXPECT_TRUE(groupResult.accepted);
    EXPECT_TRUE(groupResult.applied);
    EXPECT_FALSE(groupResult.buffered);
    EXPECT_EQ(router.pendingCount(), 0);
    EXPECT_EQ(applied, QStringList({QStringLiteral("p-ready"), QStringLiteral("g-ready")}));
}

TEST(IncomingMessageRouterTest, DedupesPrivateAndGroupMessages)
{
    IncomingMessageRouter router;

    EXPECT_TRUE(router.bufferPrivateMessage(notReady(), privateMessage(1, 2, QStringLiteral("p1"), 1000)));
    EXPECT_TRUE(router.bufferPrivateMessage(notReady(), privateMessage(1, 2, QStringLiteral("p1"), 1000)));
    EXPECT_TRUE(router.bufferGroupMessage(notReady(), groupMessage(7001, QStringLiteral("g1"), 1001)));
    EXPECT_TRUE(router.bufferGroupMessage(notReady(), groupMessage(7001, QStringLiteral("g1"), 1001)));

    EXPECT_EQ(router.pendingCount(), 2);
}

TEST(IncomingMessageRouterTest, DropsOldestWhenBufferIsFull)
{
    IncomingMessageRouter router;
    QStringList applied;

    for (qsizetype index = 0; index < IncomingMessageRouter::maxMessages; ++index)
    {
        ASSERT_TRUE(router.bufferPrivateMessage(notReady(),
                                                privateMessage(1, 2, QStringLiteral("p%1").arg(index), 1000 + index)));
    }
    EXPECT_EQ(router.pendingCount(), IncomingMessageRouter::maxMessages);

    EXPECT_TRUE(router.bufferGroupMessage(
        notReady(),
        groupMessage(7001, QStringLiteral("g-overflow"), 1000 + IncomingMessageRouter::maxMessages)));
    EXPECT_EQ(router.pendingCount(), IncomingMessageRouter::maxMessages);
    EXPECT_TRUE(router.bufferPrivateMessage(notReady(), privateMessage(1, 2, QStringLiteral("p0"), 999)));
    EXPECT_EQ(router.pendingCount(), IncomingMessageRouter::maxMessages);

    IncomingMessageRouterDispatch dispatch;
    dispatch.applyPrivateMessage = [&applied](std::shared_ptr<TextChatMsg> msg)
    {
        ASSERT_TRUE(msg);
        ASSERT_FALSE(msg->_chat_msgs.empty());
        applied << msg->_chat_msgs.front()->_msg_id;
    };
    dispatch.applyGroupMessage = [&applied](std::shared_ptr<GroupChatMsg> msg)
    {
        ASSERT_TRUE(msg);
        ASSERT_TRUE(msg->_msg);
        applied << msg->_msg->_msg_id;
    };

    EXPECT_EQ(router.flush(ready(), dispatch), IncomingMessageRouter::maxMessages);
    ASSERT_FALSE(applied.isEmpty());
    EXPECT_EQ(applied.front(), QStringLiteral("p0"));
    EXPECT_EQ(applied.back(), QStringLiteral("g-overflow"));
    EXPECT_FALSE(applied.contains(QStringLiteral("p1")));
}

TEST(IncomingMessageRouterTest, FlushesByCreatedAtThenArrivalOrder)
{
    IncomingMessageRouter router;
    QStringList applied;

    ASSERT_TRUE(router.bufferPrivateMessage(notReady(), privateMessage(1, 2, QStringLiteral("p-late"), 3000)));
    ASSERT_TRUE(router.bufferGroupMessage(notReady(), groupMessage(7001, QStringLiteral("g-early"), 1000)));
    ASSERT_TRUE(router.bufferPrivateMessage(notReady(), privateMessage(1, 3, QStringLiteral("p-mid-a"), 2000)));
    ASSERT_TRUE(router.bufferGroupMessage(notReady(), groupMessage(7002, QStringLiteral("g-mid-b"), 2000)));

    IncomingMessageRouterDispatch dispatch;
    dispatch.applyPrivateMessage = [&applied](std::shared_ptr<TextChatMsg> msg)
    {
        ASSERT_TRUE(msg);
        ASSERT_FALSE(msg->_chat_msgs.empty());
        applied << msg->_chat_msgs.front()->_msg_id;
    };
    dispatch.applyGroupMessage = [&applied](std::shared_ptr<GroupChatMsg> msg)
    {
        ASSERT_TRUE(msg);
        ASSERT_TRUE(msg->_msg);
        applied << msg->_msg->_msg_id;
    };

    EXPECT_EQ(router.flush(ready(), dispatch), 4);
    EXPECT_EQ(
        applied,
        QStringList({QStringLiteral("g-early"),
                                    QStringLiteral("p-mid-a"), QStringLiteral("g-mid-b"), QStringLiteral("p-late")}));
    EXPECT_EQ(router.pendingCount(), 0);
}

TEST(IncomingMessageRouterTest, FlushWaitsUntilReadyAndClearDropsBufferedMessages)
{
    IncomingMessageRouter router;
    int applied = 0;

    ASSERT_TRUE(router.bufferPrivateMessage(notReady(), privateMessage(1, 2, QStringLiteral("p1"), 1000)));
    IncomingMessageRouterDispatch dispatch;
    dispatch.applyPrivateMessage = [&applied](std::shared_ptr<TextChatMsg>)
    {
        ++applied;
    };

    EXPECT_EQ(router.flush(notReady(), dispatch), 0);
    EXPECT_EQ(router.pendingCount(), 1);

    router.clear();
    EXPECT_EQ(router.pendingCount(), 0);
    EXPECT_EQ(router.flush(ready(), dispatch), 0);
    EXPECT_EQ(applied, 0);
}

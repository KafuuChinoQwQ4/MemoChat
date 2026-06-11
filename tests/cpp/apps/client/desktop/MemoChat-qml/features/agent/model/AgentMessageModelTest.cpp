#include "AgentMessageModel.h"

#include <gtest/gtest.h>

#include <QModelIndex>
#include <QSignalSpy>
#include <QVariant>

namespace
{
QVariant roleData(const AgentMessageModel& model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}
} // namespace

TEST(AgentMessageModelTest, FinalizeClearsStreamingStateAndKeepsAnswer)
{
    AgentMessageModel model;
    QSignalSpy streamingSpy(&model, &AgentMessageModel::streamingStateChanged);

    EXPECT_FALSE(model.hasStreamingMessage());
    model.appendAIMessage(QStringLiteral("ai-1"), QStringLiteral("test-model"));
    ASSERT_EQ(model.rowCount(), 1);
    EXPECT_TRUE(roleData(model, 0, AgentMessageModel::IsStreamingRole).toBool());
    EXPECT_TRUE(model.hasStreamingMessage());
    EXPECT_EQ(streamingSpy.count(), 1);

    model.updateStreamingContent(QStringLiteral("ai-1"), QStringLiteral("hello"));
    EXPECT_EQ(roleData(model, 0, AgentMessageModel::ContentRole).toString(), QStringLiteral("hello"));
    EXPECT_EQ(roleData(model, 0, AgentMessageModel::StreamingContentRole).toString(), QStringLiteral("hello"));
    EXPECT_TRUE(roleData(model, 0, AgentMessageModel::IsStreamingRole).toBool());

    model.finalizeAIMessage(QStringLiteral("ai-1"));
    EXPECT_FALSE(roleData(model, 0, AgentMessageModel::IsStreamingRole).toBool());
    EXPECT_FALSE(model.hasStreamingMessage());
    EXPECT_EQ(roleData(model, 0, AgentMessageModel::ContentRole).toString(), QStringLiteral("hello"));
    EXPECT_EQ(roleData(model, 0, AgentMessageModel::StreamingContentRole).toString(), QStringLiteral("hello"));
    EXPECT_EQ(streamingSpy.count(), 2);
}

TEST(AgentMessageModelTest, FinalizeSeparatesThinkingBlockFromVisibleAnswer)
{
    AgentMessageModel model;

    model.appendAIMessage(QStringLiteral("ai-1"), QStringLiteral("test-model"));
    model.updateStreamingContent(QStringLiteral("ai-1"),
                                                QStringLiteral("<think>hidden reasoning</think>\nvisible answer"));
    model.finalizeAIMessage(QStringLiteral("ai-1"));

    EXPECT_FALSE(roleData(model, 0, AgentMessageModel::IsStreamingRole).toBool());
    EXPECT_EQ(roleData(model, 0, AgentMessageModel::ThinkingContentRole).toString(),
              QStringLiteral("hidden reasoning"));
    EXPECT_EQ(roleData(model, 0, AgentMessageModel::ContentRole).toString(), QStringLiteral("visible answer"));
}

TEST(AgentMessageModelTest, ErrorAndClearBothResetStreamingAggregateState)
{
    AgentMessageModel model;
    QSignalSpy streamingSpy(&model, &AgentMessageModel::streamingStateChanged);

    model.appendAIMessage(QStringLiteral("ai-1"), QStringLiteral("test-model"));
    EXPECT_TRUE(model.hasStreamingMessage());
    EXPECT_EQ(streamingSpy.count(), 1);
    model.setError(QStringLiteral("ai-1"), QStringLiteral("failed"));
    EXPECT_FALSE(model.hasStreamingMessage());
    EXPECT_FALSE(roleData(model, 0, AgentMessageModel::IsStreamingRole).toBool());
    EXPECT_EQ(streamingSpy.count(), 2);

    model.appendAIMessage(QStringLiteral("ai-2"), QStringLiteral("test-model"));
    EXPECT_TRUE(model.hasStreamingMessage());
    EXPECT_EQ(streamingSpy.count(), 3);
    model.clear();
    EXPECT_EQ(model.rowCount(), 0);
    EXPECT_FALSE(model.hasStreamingMessage());
    EXPECT_EQ(streamingSpy.count(), 4);
}

TEST(AgentMessageModelTest, FinalizeAllStreamingMessagesClearsEveryPendingRow)
{
    AgentMessageModel model;
    QSignalSpy streamingSpy(&model, &AgentMessageModel::streamingStateChanged);

    model.appendAIMessage(QStringLiteral("ai-1"), QStringLiteral("test-model"));
    model.updateStreamingContent(QStringLiteral("ai-1"), QStringLiteral("first answer"));
    model.appendUserMessage(QStringLiteral("next prompt"));
    model.appendAIMessage(QStringLiteral("ai-2"), QStringLiteral("test-model"));
    model.updateStreamingContent(QStringLiteral("ai-2"), QStringLiteral("second answer"));

    ASSERT_EQ(model.rowCount(), 3);
    EXPECT_TRUE(model.hasStreamingMessage());
    EXPECT_TRUE(roleData(model, 0, AgentMessageModel::IsStreamingRole).toBool());
    EXPECT_TRUE(roleData(model, 2, AgentMessageModel::IsStreamingRole).toBool());

    model.finalizeAllStreamingMessages();

    EXPECT_FALSE(model.hasStreamingMessage());
    EXPECT_FALSE(roleData(model, 0, AgentMessageModel::IsStreamingRole).toBool());
    EXPECT_FALSE(roleData(model, 2, AgentMessageModel::IsStreamingRole).toBool());
    EXPECT_EQ(roleData(model, 0, AgentMessageModel::ContentRole).toString(), QStringLiteral("first answer"));
    EXPECT_EQ(roleData(model, 2, AgentMessageModel::ContentRole).toString(), QStringLiteral("second answer"));
    EXPECT_EQ(streamingSpy.count(), 3);
}

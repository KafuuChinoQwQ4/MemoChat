#include "MessageMutationCommandService.h"

#include <gtest/gtest.h>

#include <QJsonDocument>
#include <QJsonObject>

namespace
{
constexpr int kSelfUid = 1001;
constexpr int kPeerUid = 2002;
constexpr qint64 kGroupId = 7001;
constexpr int kEditGroupMsgReq = 1065;
constexpr int kRevokeGroupMsgReq = 1067;
constexpr int kForwardGroupMsgReq = 1069;
constexpr int kEditPrivateMsgReq = 1077;
constexpr int kRevokePrivateMsgReq = 1079;
constexpr int kForwardPrivateMsgReq = 1083;

QJsonObject objectFrom(const QByteArray& payload)
{
    return QJsonDocument::fromJson(payload).object();
}

struct MutationHarness
{
    int privateMessageExistsCalls = 0;
    QString privateMessageExistsMsgId;
    bool privateMessageExistsResult = true;
    int canRevokeMessageCalls = 0;
    QString canRevokeMessageMsgId;
    bool canRevokeMessageResult = true;
    int privateStatusCalls = 0;
    int groupStatusCalls = 0;
    QString statusText;
    bool statusError = false;
    int dispatchCalls = 0;
    int dispatchedReqId = 0;
    QByteArray dispatchedPayload;

    MessageMutationCommandDependencies dependencies()
    {
        MessageMutationCommandDependencies deps;
        deps.privateMessageExists = [this](const QString& msgId)
        {
            ++privateMessageExistsCalls;
            privateMessageExistsMsgId = msgId;
            return privateMessageExistsResult;
        };
        deps.canRevokeMessage = [this](const QString& msgId)
        {
            ++canRevokeMessageCalls;
            canRevokeMessageMsgId = msgId;
            return canRevokeMessageResult;
        };
        deps.setPrivateStatus = [this](const QString& text, bool isError)
        {
            ++privateStatusCalls;
            statusText = text;
            statusError = isError;
        };
        deps.setGroupStatus = [this](const QString& text, bool isError)
        {
            ++groupStatusCalls;
            statusText = text;
            statusError = isError;
        };
        deps.dispatchPayload = [this](int reqId, const QByteArray& payload)
        {
            ++dispatchCalls;
            dispatchedReqId = reqId;
            dispatchedPayload = payload;
        };
        return deps;
    }
};

MessageMutationCommandRequest privateRequest(MessageMutationCommand command)
{
    MessageMutationCommandRequest request;
    request.selfUid = kSelfUid;
    request.command = command;
    request.target.peerUid = kPeerUid;
    request.msgId = QStringLiteral(" msg-1 ");
    request.text = QStringLiteral(" edited text ");
    return request;
}

MessageMutationCommandRequest groupRequest(MessageMutationCommand command)
{
    MessageMutationCommandRequest request;
    request.selfUid = kSelfUid;
    request.command = command;
    request.target.groupId = kGroupId;
    request.msgId = QStringLiteral(" group-msg-1 ");
    request.text = QStringLiteral(" group edit ");
    return request;
}
} // namespace

TEST(MessageMutationCommandServiceTest, PrivateEditBuildsPayloadAndDispatchesPrivateReqId)
{
    MutationHarness harness;
    const MessageMutationCommandResult result =
        MessageMutationCommandService::run(privateRequest(MessageMutationCommand::Edit), harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.dispatched);
    EXPECT_EQ(result.reqId, kEditPrivateMsgReq);
    EXPECT_EQ(harness.dispatchCalls, 1);
    EXPECT_EQ(harness.dispatchedReqId, kEditPrivateMsgReq);
    EXPECT_EQ(harness.privateStatusCalls, 0);
    EXPECT_EQ(harness.groupStatusCalls, 0);

    const QJsonObject payload = objectFrom(harness.dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), kSelfUid);
    EXPECT_EQ(payload.value(QStringLiteral("msgid")).toString(), QStringLiteral("msg-1"));
    EXPECT_EQ(payload.value(QStringLiteral("content")).toString(), QStringLiteral("edited text"));
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), kPeerUid);
    EXPECT_FALSE(payload.contains(QStringLiteral("groupid")));
}

TEST(MessageMutationCommandServiceTest, GroupRevokeBuildsPayloadAndDispatchesGroupReqId)
{
    MutationHarness harness;
    const MessageMutationCommandResult result =
        MessageMutationCommandService::run(groupRequest(MessageMutationCommand::Revoke), harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reqId, kRevokeGroupMsgReq);
    EXPECT_EQ(harness.canRevokeMessageCalls, 1);
    EXPECT_EQ(harness.canRevokeMessageMsgId, QStringLiteral("group-msg-1"));
    ASSERT_EQ(harness.dispatchCalls, 1);

    const QJsonObject payload = objectFrom(harness.dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), kSelfUid);
    EXPECT_EQ(payload.value(QStringLiteral("msgid")).toString(), QStringLiteral("group-msg-1"));
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), kGroupId);
    EXPECT_FALSE(payload.contains(QStringLiteral("peer_uid")));
    EXPECT_FALSE(payload.contains(QStringLiteral("content")));
}

TEST(MessageMutationCommandServiceTest, PrivateRevokeBuildsPayloadAndDispatchesPrivateReqId)
{
    MutationHarness harness;
    const MessageMutationCommandResult result =
        MessageMutationCommandService::run(privateRequest(MessageMutationCommand::Revoke), harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reqId, kRevokePrivateMsgReq);
    EXPECT_EQ(harness.canRevokeMessageCalls, 1);
    EXPECT_EQ(harness.canRevokeMessageMsgId, QStringLiteral("msg-1"));
    ASSERT_EQ(harness.dispatchCalls, 1);

    const QJsonObject payload = objectFrom(harness.dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), kSelfUid);
    EXPECT_EQ(payload.value(QStringLiteral("msgid")).toString(), QStringLiteral("msg-1"));
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), kPeerUid);
    EXPECT_FALSE(payload.contains(QStringLiteral("groupid")));
    EXPECT_FALSE(payload.contains(QStringLiteral("content")));
}

TEST(MessageMutationCommandServiceTest, ForwardAddsClientMessageIdAndChecksPrivateSourceMessage)
{
    MutationHarness harness;
    const MessageMutationCommandResult result =
        MessageMutationCommandService::run(privateRequest(MessageMutationCommand::Forward), harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reqId, kForwardPrivateMsgReq);
    EXPECT_EQ(harness.privateMessageExistsCalls, 1);
    EXPECT_EQ(harness.privateMessageExistsMsgId, QStringLiteral("msg-1"));

    const QJsonObject payload = objectFrom(harness.dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), kSelfUid);
    EXPECT_EQ(payload.value(QStringLiteral("msgid")).toString(), QStringLiteral("msg-1"));
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), kPeerUid);
    EXPECT_FALSE(payload.value(QStringLiteral("client_msg_id")).toString().isEmpty());
    EXPECT_FALSE(payload.contains(QStringLiteral("content")));
}

TEST(MessageMutationCommandServiceTest, GroupForwardDoesNotRequirePrivateSourceCheck)
{
    MutationHarness harness;
    const MessageMutationCommandResult result =
        MessageMutationCommandService::run(groupRequest(MessageMutationCommand::Forward), harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.reqId, kForwardGroupMsgReq);
    EXPECT_EQ(harness.privateMessageExistsCalls, 0);
    ASSERT_EQ(harness.dispatchCalls, 1);

    const QJsonObject payload = objectFrom(harness.dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), kGroupId);
    EXPECT_FALSE(payload.value(QStringLiteral("client_msg_id")).toString().isEmpty());
}

TEST(MessageMutationCommandServiceTest, EmptyEditTextUsesGroupStatusWhenTargetIsGroup)
{
    MutationHarness harness;
    MessageMutationCommandRequest request = groupRequest(MessageMutationCommand::Edit);
    request.text = QStringLiteral("   ");

    const MessageMutationCommandResult result = MessageMutationCommandService::run(request, harness.dependencies());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorText, QStringLiteral("编辑消息参数非法"));
    EXPECT_TRUE(result.errorIsGroup);
    EXPECT_EQ(harness.groupStatusCalls, 1);
    EXPECT_EQ(harness.privateStatusCalls, 0);
    EXPECT_TRUE(harness.statusError);
    EXPECT_EQ(harness.dispatchCalls, 0);
}

TEST(MessageMutationCommandServiceTest, MissingDialogUsesPrivateShellStatus)
{
    MutationHarness harness;
    MessageMutationCommandRequest request = privateRequest(MessageMutationCommand::Revoke);
    request.target.peerUid = 0;

    const MessageMutationCommandResult result = MessageMutationCommandService::run(request, harness.dependencies());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorText, QStringLiteral("请选择会话"));
    EXPECT_FALSE(result.errorIsGroup);
    EXPECT_EQ(harness.privateStatusCalls, 1);
    EXPECT_EQ(harness.groupStatusCalls, 0);
    EXPECT_EQ(harness.dispatchCalls, 0);
}

TEST(MessageMutationCommandServiceTest, MissingPrivateForwardSourceMessageRejectsBeforeDispatch)
{
    MutationHarness harness;
    harness.privateMessageExistsResult = false;

    const MessageMutationCommandResult result =
        MessageMutationCommandService::run(privateRequest(MessageMutationCommand::Forward), harness.dependencies());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorText, QStringLiteral("未找到要转发的消息"));
    EXPECT_EQ(harness.privateMessageExistsCalls, 1);
    EXPECT_EQ(harness.privateStatusCalls, 1);
    EXPECT_EQ(harness.groupStatusCalls, 0);
    EXPECT_EQ(harness.dispatchCalls, 0);
}

TEST(MessageMutationCommandServiceTest, GroupRevokeRejectsWhenMessageIsNotSelfOrExpired)
{
    MutationHarness harness;
    harness.canRevokeMessageResult = false;

    const MessageMutationCommandResult result =
        MessageMutationCommandService::run(groupRequest(MessageMutationCommand::Revoke), harness.dependencies());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorText, QStringLiteral("只能撤回5分钟内自己发送的消息"));
    EXPECT_TRUE(result.errorIsGroup);
    EXPECT_EQ(harness.canRevokeMessageCalls, 1);
    EXPECT_EQ(harness.groupStatusCalls, 1);
    EXPECT_EQ(harness.privateStatusCalls, 0);
    EXPECT_EQ(harness.dispatchCalls, 0);
}

TEST(MessageMutationCommandServiceTest, PrivateRevokeRejectsWhenMessageIsNotSelfOrExpired)
{
    MutationHarness harness;
    harness.canRevokeMessageResult = false;

    const MessageMutationCommandResult result =
        MessageMutationCommandService::run(privateRequest(MessageMutationCommand::Revoke), harness.dependencies());

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.errorText, QStringLiteral("只能撤回5分钟内自己发送的消息"));
    EXPECT_FALSE(result.errorIsGroup);
    EXPECT_EQ(harness.canRevokeMessageCalls, 1);
    EXPECT_EQ(harness.privateStatusCalls, 1);
    EXPECT_EQ(harness.groupStatusCalls, 0);
    EXPECT_EQ(harness.dispatchCalls, 0);
}

TEST(MessageMutationCommandServiceTest, EditContentIsCappedAtProtocolLimit)
{
    MutationHarness harness;
    MessageMutationCommandRequest request = privateRequest(MessageMutationCommand::Edit);
    request.text = QString(5000, QLatin1Char('x'));

    const MessageMutationCommandResult result = MessageMutationCommandService::run(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    const QJsonObject payload = objectFrom(harness.dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("content")).toString().size(), 4096);
}

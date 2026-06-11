#include "PrivateChatSendService.h"

#include "ChatMessageModel.h"
#include "FriendListModel.h"

#include <gtest/gtest.h>

#include <QString>

#include <memory>
#include <vector>

QString gate_url_prefix;
QString gate_media_url_prefix;

namespace
{
constexpr int kSelfUid = 1001;
constexpr int kPeerUid = 2002;

std::shared_ptr<FriendInfo>
makeFriend(int uid,
           const QString& name = QStringLiteral("Peer"),
                                                const QString& lastMsg = QStringLiteral("old preview"),
                                                                                        int unreadCount = 3)
{
    auto info = std::make_shared<FriendInfo>(
        uid,
        name,
        name,
        QStringLiteral("qrc:/res/head_1.png"), 0, QStringLiteral("desc"), name, lastMsg, QStringLiteral("u000000001"));
    info->_dialog_type = QStringLiteral("private");
    info->_unread_count = unreadCount;
    info->_last_msg_ts = 1234;
    return info;
}

QVariant messageValue(const ChatMessageModel& model, int role)
{
    return model.data(model.index(0, 0), role);
}

QVariantMap firstFriend(const FriendListModel& model)
{
    return model.get(0);
}

struct SendHarness
{
    ChatMessageModel messageModel;
    FriendListModel chatListModel;
    FriendListModel dialogListModel;
    qint64 historyBeforeTs = -1;
    QString historyBeforeMsgId = QStringLiteral("stale");
    int dispatchCalls = 0;
    int appendFriendCalls = 0;
    int appendedPeerUid = 0;
    std::vector<std::shared_ptr<TextChatData>> appendedMessages;
    OutgoingChatPacket dispatchedPacket;
    bool dispatchResult = true;
    QString dispatchError;

    SendHarness()
    {
        chatListModel.setFriends({makeFriend(kPeerUid)});
        dialogListModel.setFriends({makeFriend(kPeerUid, QStringLiteral("Dialog Peer"))});
    }

    PrivateChatSendDependencies dependencies()
    {
        PrivateChatSendDependencies deps;
        deps.selfUid = kSelfUid;
        deps.dispatchPacket = [this](const OutgoingChatPacket& packet, QString* errorText)
        {
            ++dispatchCalls;
            dispatchedPacket = packet;
            if (!dispatchError.isEmpty() && errorText)
            {
                *errorText = dispatchError;
            }
            return dispatchResult;
        };
        deps.appendFriendMessages = [this](int peerUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
        {
            ++appendFriendCalls;
            appendedPeerUid = peerUid;
            appendedMessages = messages;
        };
        deps.cacheStore = nullptr;
        deps.messageModel = &messageModel;
        deps.chatListModel = &chatListModel;
        deps.dialogListModel = &dialogListModel;
        deps.historyBeforeTs = &historyBeforeTs;
        deps.historyBeforeMsgId = &historyBeforeMsgId;
        return deps;
    }
};

PrivateChatSendRequest makeRequest()
{
    PrivateChatSendRequest request;
    request.peerUid = kPeerUid;
    request.content = QStringLiteral("hello from tests");
    request.previewText = QStringLiteral("preview from tests");
    return request;
}
} // namespace

TEST(PrivateChatSendServiceTest, MissingPeerRejectsWithoutDispatch)
{
    SendHarness harness;
    PrivateChatSendRequest request = makeRequest();
    request.peerUid = 0;
    PrivateChatSendDependencies deps = harness.dependencies();
    QString errorText;

    const bool sent = PrivateChatSendService::trySend(request, deps, &errorText);

    EXPECT_FALSE(sent);
    EXPECT_EQ(harness.dispatchCalls, 0);
    EXPECT_EQ(harness.appendFriendCalls, 0);
    EXPECT_EQ(harness.messageModel.rowCount(), 0);
    EXPECT_EQ(
        firstFriend(harness.chatListModel).value(QStringLiteral("lastMsg")).toString(), QStringLiteral("old preview"));
    EXPECT_EQ(firstFriend(harness.dialogListModel).value(QStringLiteral("unreadCount")).toInt(), 3);
    EXPECT_EQ(harness.historyBeforeTs, -1);
    EXPECT_EQ(harness.historyBeforeMsgId, QStringLiteral("stale"));
}

TEST(PrivateChatSendServiceTest, MissingSelfRejectsWithoutDispatch)
{
    SendHarness harness;
    PrivateChatSendRequest request = makeRequest();
    PrivateChatSendDependencies deps = harness.dependencies();
    deps.selfUid = 0;
    QString errorText;

    const bool sent = PrivateChatSendService::trySend(request, deps, &errorText);

    EXPECT_FALSE(sent);
    EXPECT_EQ(harness.dispatchCalls, 0);
    EXPECT_EQ(harness.appendFriendCalls, 0);
    EXPECT_EQ(harness.messageModel.rowCount(), 0);
    EXPECT_EQ(
        firstFriend(harness.chatListModel).value(QStringLiteral("lastMsg")).toString(), QStringLiteral("old preview"));
    EXPECT_EQ(firstFriend(harness.dialogListModel).value(QStringLiteral("unreadCount")).toInt(), 3);
    EXPECT_EQ(harness.historyBeforeTs, -1);
    EXPECT_EQ(harness.historyBeforeMsgId, QStringLiteral("stale"));
}

TEST(PrivateChatSendServiceTest, DispatchFailureDoesNotAppendOptimisticMessage)
{
    SendHarness harness;
    harness.dispatchResult = false;
    harness.dispatchError = QStringLiteral("transport failed");
    PrivateChatSendRequest request = makeRequest();
    PrivateChatSendDependencies deps = harness.dependencies();
    QString errorText;

    const bool sent = PrivateChatSendService::trySend(request, deps, &errorText);

    EXPECT_FALSE(sent);
    EXPECT_EQ(errorText, QStringLiteral("transport failed"));
    EXPECT_EQ(harness.dispatchCalls, 1);
    EXPECT_EQ(harness.dispatchedPacket.fromUid, kSelfUid);
    EXPECT_EQ(harness.dispatchedPacket.toUid, kPeerUid);
    EXPECT_EQ(harness.dispatchedPacket.content, request.content);
    EXPECT_EQ(harness.appendFriendCalls, 0);
    EXPECT_TRUE(harness.appendedMessages.empty());
    EXPECT_EQ(harness.messageModel.rowCount(), 0);
    EXPECT_EQ(
        firstFriend(harness.chatListModel).value(QStringLiteral("lastMsg")).toString(), QStringLiteral("old preview"));
    EXPECT_EQ(firstFriend(harness.dialogListModel).value(QStringLiteral("unreadCount")).toInt(), 3);
    EXPECT_EQ(harness.historyBeforeTs, -1);
    EXPECT_EQ(harness.historyBeforeMsgId, QStringLiteral("stale"));
}

TEST(PrivateChatSendServiceTest, SuccessfulSendDispatchesAndUpdatesOptimisticPrivateState)
{
    SendHarness harness;
    PrivateChatSendRequest request = makeRequest();
    PrivateChatSendDependencies deps = harness.dependencies();
    QString errorText;

    const bool sent = PrivateChatSendService::trySend(request, deps, &errorText);

    EXPECT_TRUE(sent);
    EXPECT_TRUE(errorText.isEmpty());
    EXPECT_EQ(harness.dispatchCalls, 1);
    EXPECT_EQ(harness.dispatchedPacket.fromUid, kSelfUid);
    EXPECT_EQ(harness.dispatchedPacket.toUid, kPeerUid);
    EXPECT_EQ(harness.dispatchedPacket.content, request.content);
    EXPECT_FALSE(harness.dispatchedPacket.msgId.isEmpty());
    EXPECT_GT(harness.dispatchedPacket.createdAt, 0);

    ASSERT_EQ(harness.appendFriendCalls, 1);
    EXPECT_EQ(harness.appendedPeerUid, kPeerUid);
    ASSERT_EQ(harness.appendedMessages.size(), 1U);
    const std::shared_ptr<TextChatData>& optimistic = harness.appendedMessages.front();
    ASSERT_NE(optimistic, nullptr);
    EXPECT_EQ(optimistic->_msg_id, harness.dispatchedPacket.msgId);
    EXPECT_EQ(optimistic->_msg_content, request.content);
    EXPECT_EQ(optimistic->_from_uid, kSelfUid);
    EXPECT_EQ(optimistic->_to_uid, kPeerUid);
    EXPECT_EQ(optimistic->_created_at, harness.dispatchedPacket.createdAt);
    EXPECT_EQ(optimistic->_msg_state, QStringLiteral("sending"));

    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, ChatMessageModel::MsgIdRole).toString(),
              harness.dispatchedPacket.msgId);
    EXPECT_EQ(messageValue(harness.messageModel, ChatMessageModel::RawContentRole).toString(), request.content);
    EXPECT_EQ(messageValue(harness.messageModel, ChatMessageModel::ContentRole).toString(), request.content);
    EXPECT_EQ(messageValue(harness.messageModel, ChatMessageModel::FromUidRole).toInt(), kSelfUid);
    EXPECT_EQ(messageValue(harness.messageModel, ChatMessageModel::ToUidRole).toInt(), kPeerUid);
    EXPECT_TRUE(messageValue(harness.messageModel, ChatMessageModel::OutgoingRole).toBool());
    EXPECT_EQ(messageValue(harness.messageModel, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("sending"));
    EXPECT_EQ(messageValue(harness.messageModel, ChatMessageModel::CreatedAtRole).toLongLong(),
              harness.dispatchedPacket.createdAt);

    const QVariantMap chatEntry = firstFriend(harness.chatListModel);
    const QVariantMap dialogEntry = firstFriend(harness.dialogListModel);
    EXPECT_EQ(chatEntry.value(QStringLiteral("lastMsg")).toString(), request.previewText);
    EXPECT_EQ(dialogEntry.value(QStringLiteral("lastMsg")).toString(), request.previewText);
    EXPECT_EQ(chatEntry.value(QStringLiteral("unreadCount")).toInt(), 0);
    EXPECT_EQ(dialogEntry.value(QStringLiteral("unreadCount")).toInt(), 0);
    EXPECT_EQ(chatEntry.value(QStringLiteral("lastMsgTs")).toLongLong(), harness.dispatchedPacket.createdAt);
    EXPECT_EQ(dialogEntry.value(QStringLiteral("lastMsgTs")).toLongLong(), harness.dispatchedPacket.createdAt);

    EXPECT_EQ(harness.historyBeforeTs, harness.dispatchedPacket.createdAt);
    EXPECT_EQ(harness.historyBeforeMsgId, harness.dispatchedPacket.msgId);
}

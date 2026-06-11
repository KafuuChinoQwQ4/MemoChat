#include "PrivateChatHistoryService.h"

#include "ChatMessageModel.h"
#include "FriendListModel.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QVariantMap>

#include <algorithm>
#include <map>
#include <memory>
#include <vector>

QString gate_url_prefix;
QString gate_media_url_prefix;

namespace
{
constexpr int kSelfUid = 1001;
constexpr int kPeerUid = 2002;
constexpr int kOtherPeerUid = 3003;
constexpr int kProtocolSuccess = 0;
constexpr int kProtocolJsonError = 1;
constexpr qint64 kHistoryTs1 = 1700000001000LL;
constexpr qint64 kHistoryTs2 = 1700000002000LL;
constexpr qint64 kHistoryTs3 = 1700000003000LL;
constexpr qint64 kBackgroundTs = 1700000004000LL;
constexpr qint64 kCurrentTs = 1700000009000LL;

std::shared_ptr<FriendInfo> makeFriendWithOptions(int uid, const QString& name, const QString& lastMsg, int unreadCount)
{
    auto info = std::make_shared<FriendInfo>(
        uid,
        name,
        name,
        QStringLiteral("qrc:/res/head_1.png"), 0, QStringLiteral("desc"), name, lastMsg, QStringLiteral("u000000001"));
    info->_dialog_type = QStringLiteral("private");
    info->_unread_count = unreadCount;
    info->_last_msg_ts = 500;
    return info;
}

std::shared_ptr<FriendInfo> makeFriend(int uid)
{
    return makeFriendWithOptions(uid, QStringLiteral("Peer"), QStringLiteral("old preview"), 4);
}

std::shared_ptr<FriendInfo> makeFriend(int uid, const QString& name)
{
    return makeFriendWithOptions(uid, name, QStringLiteral("old preview"), 4);
}

std::shared_ptr<TextChatData>
makeMessage(const QString& msgId, int fromUid, int toUid, const QString& content, qint64 createdAt)
{
    return std::make_shared<TextChatData>(msgId,
                                          content,
                                          fromUid,
                                          toUid,
                                          QString(),
                                          createdAt,
                                          QString(),
                                          QStringLiteral("sent"));
}

QJsonObject historyMessage(const QString& msgId, int fromUid, int toUid, const QString& content, qint64 createdAt)
{
    QJsonObject obj;
    obj.insert(QStringLiteral("msgid"), msgId);
    obj.insert(QStringLiteral("fromuid"), fromUid);
    obj.insert(QStringLiteral("touid"), toUid);
    obj.insert(QStringLiteral("content"), content);
    obj.insert(QStringLiteral("created_at"), createdAt);
    return obj;
}

QJsonObject historyPayload(int peerUid, const QJsonArray& messages, bool hasMore = true, int error = kProtocolSuccess)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("peer_uid"), peerUid);
    payload.insert(QStringLiteral("error"), error);
    payload.insert(QStringLiteral("messages"), messages);
    payload.insert(QStringLiteral("has_more"), hasMore);
    return payload;
}

QVariant messageValue(const ChatMessageModel& model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

QVariantMap firstFriend(const FriendListModel& model)
{
    return model.get(0);
}

struct HistoryHarness
{
    ChatMessageModel messageModel;
    FriendListModel chatListModel;
    FriendListModel dialogListModel;
    PrivateHistoryRequestState state;
    std::map<int, std::vector<std::shared_ptr<TextChatData>>> friendMessages;
    int loadingCalls = 0;
    bool lastLoading = true;
    int canLoadCalls = 0;
    bool lastCanLoad = true;
    int appendCalls = 0;
    int appendedPeerUid = 0;
    std::vector<std::shared_ptr<TextChatData>> appendedMessages;
    int readAckCalls = 0;
    int readAckPeerUid = 0;
    qint64 readAckTs = 0;

    HistoryHarness()
    {
        chatListModel.setFriends({makeFriend(kPeerUid), makeFriend(kOtherPeerUid, QStringLiteral("Other"))});
        dialogListModel.setFriends({makeFriend(kPeerUid), makeFriend(kOtherPeerUid, QStringLiteral("Other"))});
        state.beforeTs = 9999;
        state.beforeMsgId = QStringLiteral("before");
        state.pendingBeforeTs = 0;
        state.pendingBeforeMsgId.clear();
        state.pendingPeerUid = kPeerUid;
    }

    PrivateHistoryResponseDependencies dependencies()
    {
        PrivateHistoryResponseDependencies deps;
        deps.historyState = &state;
        deps.cacheStore = nullptr;
        deps.messageModel = &messageModel;
        deps.chatListModel = &chatListModel;
        deps.dialogListModel = &dialogListModel;
        deps.setLoading = [this](bool loading)
        {
            ++loadingCalls;
            lastLoading = loading;
        };
        deps.setCanLoadMore = [this](bool canLoad)
        {
            ++canLoadCalls;
            lastCanLoad = canLoad;
        };
        deps.appendFriendMessages = [this](int peerUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
        {
            ++appendCalls;
            appendedPeerUid = peerUid;
            appendedMessages = messages;
            auto& existing = friendMessages[peerUid];
            existing.insert(existing.end(), messages.begin(), messages.end());
            std::sort(existing.begin(),
                      existing.end(),
                      [](const std::shared_ptr<TextChatData>& lhs, const std::shared_ptr<TextChatData>& rhs)
                      {
                          if (lhs->_created_at != rhs->_created_at)
                          {
                              return lhs->_created_at < rhs->_created_at;
                          }
                          return lhs->_msg_id < rhs->_msg_id;
                      });
        };
        deps.friendMessages = [this](int peerUid)
        {
            return friendMessages[peerUid];
        };
        deps.requestReadAck = [this](int peerUid, qint64 readTs)
        {
            ++readAckCalls;
            readAckPeerUid = peerUid;
            readAckTs = readTs;
        };
        return deps;
    }
};

PrivateHistoryResponseRequest makeRequest(const QJsonObject& payload, int currentPeerUid, int selfUid = kSelfUid)
{
    PrivateHistoryResponseRequest request;
    request.payload = payload;
    request.currentPeerUid = currentPeerUid;
    request.selfUid = selfUid;
    return request;
}
} // namespace

TEST(PrivateChatHistoryServiceTest, PendingErrorClearsPendingAndDisablesLoadMoreWithoutMutations)
{
    HistoryHarness harness;
    harness.state.pendingBeforeMsgId = QStringLiteral("pending");
    QJsonArray messages;
    messages.append(historyMessage(QStringLiteral("m1"), kPeerUid, kSelfUid, QStringLiteral("ignored"), kHistoryTs1));

    const PrivateHistoryResponseResult result = PrivateChatHistoryService::handleResponse(
        makeRequest(historyPayload(kPeerUid, messages, true, kProtocolJsonError), kPeerUid),
        harness.dependencies());

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.pendingMatched);
    EXPECT_EQ(harness.loadingCalls, 1);
    EXPECT_FALSE(harness.lastLoading);
    EXPECT_EQ(harness.canLoadCalls, 1);
    EXPECT_FALSE(harness.lastCanLoad);
    EXPECT_EQ(harness.state.pendingPeerUid, 0);
    EXPECT_EQ(harness.state.pendingBeforeTs, 0);
    EXPECT_TRUE(harness.state.pendingBeforeMsgId.isEmpty());
    EXPECT_EQ(harness.appendCalls, 0);
    EXPECT_EQ(harness.readAckCalls, 0);
    EXPECT_EQ(harness.messageModel.rowCount(), 0);
    EXPECT_EQ(
        firstFriend(harness.chatListModel).value(QStringLiteral("lastMsg")).toString(), QStringLiteral("old preview"));
}

TEST(PrivateChatHistoryServiceTest, MissingSelfClearsPendingAndDisablesLoadMoreWithoutMutations)
{
    HistoryHarness harness;
    harness.state.pendingBeforeMsgId = QStringLiteral("pending");
    QJsonArray messages;
    messages.append(historyMessage(QStringLiteral("m1"), kPeerUid, kSelfUid, QStringLiteral("ignored"), kHistoryTs1));

    const PrivateHistoryResponseResult result =
        PrivateChatHistoryService::handleResponse(makeRequest(historyPayload(kPeerUid, messages), kPeerUid, 0),
                                                  harness.dependencies());

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.pendingMatched);
    EXPECT_EQ(harness.loadingCalls, 1);
    EXPECT_FALSE(harness.lastLoading);
    EXPECT_EQ(harness.canLoadCalls, 1);
    EXPECT_FALSE(harness.lastCanLoad);
    EXPECT_EQ(harness.state.pendingPeerUid, 0);
    EXPECT_EQ(harness.appendCalls, 0);
    EXPECT_EQ(harness.readAckCalls, 0);
    EXPECT_EQ(harness.messageModel.rowCount(), 0);
}

TEST(PrivateChatHistoryServiceTest, CurrentInitialHistoryResetsModelPreviewCursorAndRequestsReadAck)
{
    HistoryHarness harness;
    QJsonArray messages;
    messages.append(
        historyMessage(QStringLiteral("m2"), kSelfUid, kPeerUid, QStringLiteral("second self"), kHistoryTs2));
    messages.append(
        historyMessage(QStringLiteral("m1"), kPeerUid, kSelfUid, QStringLiteral("first peer"), kHistoryTs1));
    messages.append(
        historyMessage(QStringLiteral("m3"), kPeerUid, kSelfUid, QStringLiteral("latest peer"), kHistoryTs3));

    const PrivateHistoryResponseResult result =
        PrivateChatHistoryService::handleResponse(makeRequest(historyPayload(kPeerUid, messages, true), kPeerUid),
                                                  harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.currentDialog);
    EXPECT_FALSE(result.pagination);
    EXPECT_EQ(result.parsedCount, 3U);
    EXPECT_EQ(harness.loadingCalls, 1);
    EXPECT_FALSE(harness.lastLoading);
    EXPECT_EQ(harness.canLoadCalls, 1);
    EXPECT_TRUE(harness.lastCanLoad);
    EXPECT_EQ(harness.appendCalls, 1);
    EXPECT_EQ(harness.appendedPeerUid, kPeerUid);
    ASSERT_EQ(harness.appendedMessages.size(), 3U);
    EXPECT_EQ(harness.appendedMessages[0]->_msg_id, QStringLiteral("m1"));
    EXPECT_EQ(harness.appendedMessages[1]->_msg_id, QStringLiteral("m2"));
    EXPECT_EQ(harness.appendedMessages[2]->_msg_id, QStringLiteral("m3"));

    ASSERT_EQ(harness.messageModel.rowCount(), 3);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(), QStringLiteral("m1"));
    EXPECT_EQ(messageValue(harness.messageModel, 2, ChatMessageModel::MsgIdRole).toString(), QStringLiteral("m3"));
    EXPECT_EQ(
        firstFriend(harness.chatListModel).value(QStringLiteral("lastMsg")).toString(), QStringLiteral("latest peer"));
    EXPECT_EQ(firstFriend(harness.dialogListModel).value(QStringLiteral("lastMsgTs")).toLongLong(), kHistoryTs3);
    EXPECT_EQ(harness.state.beforeTs, kHistoryTs1);
    EXPECT_EQ(harness.state.beforeMsgId, QStringLiteral("m1"));
    EXPECT_EQ(harness.state.pendingPeerUid, 0);
    EXPECT_EQ(harness.readAckCalls, 1);
    EXPECT_EQ(harness.readAckPeerUid, kPeerUid);
    EXPECT_EQ(harness.readAckTs, kHistoryTs3);
}

TEST(PrivateChatHistoryServiceTest, CurrentPaginationPrependsParsedMessagesAndSyncsCursor)
{
    HistoryHarness harness;
    harness.state.beforeTs = kHistoryTs3;
    harness.state.beforeMsgId = QStringLiteral("existing");
    harness.state.pendingBeforeTs = kHistoryTs3;
    harness.state.pendingBeforeMsgId = QStringLiteral("existing");
    harness.friendMessages[kPeerUid] = {
        makeMessage(QStringLiteral("existing"), kSelfUid, kPeerUid, QStringLiteral("existing"), kHistoryTs3)};
    harness.messageModel.setMessages(harness.friendMessages[kPeerUid], kSelfUid);

    QJsonArray messages;
    messages.append(
        historyMessage(QStringLiteral("old2"), kSelfUid, kPeerUid, QStringLiteral("old self"), kHistoryTs2));
    messages.append(
        historyMessage(QStringLiteral("old1"), kPeerUid, kSelfUid, QStringLiteral("old peer"), kHistoryTs1));

    const PrivateHistoryResponseResult result =
        PrivateChatHistoryService::handleResponse(makeRequest(historyPayload(kPeerUid, messages, false), kPeerUid),
                                                  harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.pagination);
    EXPECT_EQ(harness.canLoadCalls, 1);
    EXPECT_FALSE(harness.lastCanLoad);
    ASSERT_EQ(harness.messageModel.rowCount(), 3);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(), QStringLiteral("old1"));
    EXPECT_EQ(messageValue(harness.messageModel, 2, ChatMessageModel::MsgIdRole).toString(),
              QStringLiteral("existing"));
    EXPECT_EQ(harness.state.beforeTs, kHistoryTs1);
    EXPECT_EQ(harness.state.beforeMsgId, QStringLiteral("old1"));
    EXPECT_EQ(harness.state.pendingPeerUid, 0);
    EXPECT_EQ(harness.readAckCalls, 1);
    EXPECT_EQ(harness.readAckTs, kHistoryTs1);
}

TEST(PrivateChatHistoryServiceTest, BackgroundHistoryUpdatesPreviewWithoutMutatingCurrentModel)
{
    HistoryHarness harness;
    harness.friendMessages[kPeerUid] = {
        makeMessage(QStringLiteral("current"), kSelfUid, kPeerUid, QStringLiteral("current"), kCurrentTs)};
    harness.messageModel.setMessages(harness.friendMessages[kPeerUid], kSelfUid);

    QJsonArray messages;
    messages.append(
        historyMessage(QStringLiteral("bg1"), kOtherPeerUid, kSelfUid, QStringLiteral("background"), kBackgroundTs));

    const PrivateHistoryResponseResult result =
        PrivateChatHistoryService::handleResponse(makeRequest(historyPayload(kOtherPeerUid, messages, true), kPeerUid),
                                                  harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.currentDialog);
    EXPECT_EQ(harness.loadingCalls, 0);
    EXPECT_EQ(harness.canLoadCalls, 0);
    EXPECT_EQ(harness.appendCalls, 1);
    EXPECT_EQ(harness.appendedPeerUid, kOtherPeerUid);
    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(), QStringLiteral("current"));
    const QVariantMap otherEntry = harness.dialogListModel.get(1);
    EXPECT_EQ(otherEntry.value(QStringLiteral("lastMsg")).toString(), QStringLiteral("background"));
    EXPECT_EQ(harness.readAckCalls, 0);
}

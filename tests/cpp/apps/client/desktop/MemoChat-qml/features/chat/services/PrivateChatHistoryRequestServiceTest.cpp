#include "PrivateChatHistoryRequestService.h"

#include "ChatMessageModel.h"

#include <gtest/gtest.h>

#include <QJsonDocument>
#include <QJsonObject>

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
constexpr qint64 kTs1 = 1700000001000LL;
constexpr qint64 kTs2 = 1700000002000LL;
constexpr qint64 kTs3 = 1700000003000LL;

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

PrivateHistoryPeerProfile makeProfile(const QString& name, const QString& icon = QString())
{
    PrivateHistoryPeerProfile profile;
    profile.available = true;
    profile.displayName = name;
    profile.icon = icon;
    return profile;
}

QJsonObject payloadFromBytes(const QByteArray& bytes)
{
    return QJsonDocument::fromJson(bytes).object();
}

QVariant messageValue(const ChatMessageModel& model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

void appendSorted(std::vector<std::shared_ptr<TextChatData>>& target,
                  const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    target.insert(target.end(), messages.begin(), messages.end());
    std::sort(target.begin(),
              target.end(),
              [](const std::shared_ptr<TextChatData>& lhs, const std::shared_ptr<TextChatData>& rhs)
              {
                  if (!lhs || !rhs)
                  {
                      return static_cast<bool>(lhs);
                  }
                  if (lhs->_created_at != rhs->_created_at)
                  {
                      return lhs->_created_at < rhs->_created_at;
                  }
                  return lhs->_msg_id < rhs->_msg_id;
              });
}

struct HistoryRequestHarness
{
    ChatMessageModel messageModel;
    PrivateHistoryRequestState state;
    std::map<int, PrivateHistoryPeerProfile> profiles;
    std::map<int, std::vector<std::shared_ptr<TextChatData>>> friendMessagesByPeer;
    std::vector<std::shared_ptr<TextChatData>> localCacheMessages;
    int loadRecentCalls = 0;
    int loadRecentOwnerUid = 0;
    int loadRecentPeerUid = 0;
    int loadRecentLimit = 0;
    int appendCalls = 0;
    int appendedPeerUid = 0;
    int loadingCalls = 0;
    bool lastLoading = true;
    int canLoadCalls = 0;
    bool lastCanLoad = false;
    int readAckCalls = 0;
    int readAckPeerUid = 0;
    qint64 readAckTs = 0;
    QString peerName;
    QString peerIcon;
    int dispatchCalls = 0;
    int dispatchedReqId = 0;
    QByteArray dispatchedPayload;

    HistoryRequestHarness()
    {
        state.beforeTs = 111;
        state.beforeMsgId = QStringLiteral("state-cursor");
        state.pendingPeerUid = 99;
        state.pendingBeforeTs = 222;
        state.pendingBeforeMsgId = QStringLiteral("pending-cursor");
        profiles.emplace(kPeerUid, makeProfile(QStringLiteral("Peer"), QString()));
        profiles.emplace(kOtherPeerUid, makeProfile(QStringLiteral("Other"), QStringLiteral("qrc:/res/other.png")));
    }

    PrivateHistoryLoadCurrentDependencies loadCurrentDependencies()
    {
        PrivateHistoryLoadCurrentDependencies deps;
        deps.historyState = &state;
        deps.messageModel = &messageModel;
        deps.peerProfile = [this](int peerUid)
        {
            const auto it = profiles.find(peerUid);
            return it == profiles.end() ? PrivateHistoryPeerProfile{} : it->second;
        };
        deps.friendMessages = [this](int peerUid)
        {
            const auto it = friendMessagesByPeer.find(peerUid);
            return it == friendMessagesByPeer.end() ? std::vector<std::shared_ptr<TextChatData>>{} : it->second;
        };
        deps.loadRecentMessages = [this](int ownerUid, int peerUid, int limit)
        {
            ++loadRecentCalls;
            loadRecentOwnerUid = ownerUid;
            loadRecentPeerUid = peerUid;
            loadRecentLimit = limit;
            return localCacheMessages;
        };
        deps.appendFriendMessages = [this](int peerUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
        {
            ++appendCalls;
            appendedPeerUid = peerUid;
            appendSorted(friendMessagesByPeer[peerUid], messages);
        };
        deps.setCurrentPeerName = [this](const QString& name)
        {
            peerName = name;
        };
        deps.setCurrentPeerIcon = [this](const QString& icon)
        {
            peerIcon = icon;
        };
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
        deps.requestReadAck = [this](int peerUid, qint64 readTs)
        {
            ++readAckCalls;
            readAckPeerUid = peerUid;
            readAckTs = readTs;
        };
        return deps;
    }

    PrivateHistoryRequestBuildDependencies buildDependencies()
    {
        PrivateHistoryRequestBuildDependencies deps;
        deps.historyState = &state;
        deps.setLoading = [this](bool loading)
        {
            ++loadingCalls;
            lastLoading = loading;
        };
        deps.dispatchPayload = [this](int reqId, const QByteArray& payload)
        {
            ++dispatchCalls;
            dispatchedReqId = reqId;
            dispatchedPayload = payload;
        };
        return deps;
    }

    PrivateHistoryLoadMoreDependencies loadMoreDependencies()
    {
        PrivateHistoryLoadMoreDependencies deps;
        deps.historyState = &state;
        deps.messageModel = &messageModel;
        deps.setLoading = [this](bool loading)
        {
            ++loadingCalls;
            lastLoading = loading;
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

PrivateHistoryRequestBuildRequest makeBuildRequest()
{
    PrivateHistoryRequestBuildRequest request;
    request.peerUid = kPeerUid;
    request.selfUid = kSelfUid;
    request.beforeTs = kTs2;
    request.beforeMsgId = QStringLiteral("  before-msg-2  ");
    return request;
}

PrivateHistoryLoadCurrentRequest makeLoadCurrentRequest()
{
    PrivateHistoryLoadCurrentRequest request;
    request.currentPeerUid = kPeerUid;
    request.selfUid = kSelfUid;
    return request;
}

PrivateHistoryLoadMoreRequest makeLoadMoreRequest()
{
    PrivateHistoryLoadMoreRequest request;
    request.currentPeerUid = kPeerUid;
    request.selfUid = kSelfUid;
    request.privateHistoryLoading = false;
    request.canLoadMorePrivateHistory = true;
    return request;
}
} // namespace

TEST(PrivateChatHistoryRequestServiceTest, BuildRequestDispatchesTrimmedPayloadAndMarksPending)
{
    HistoryRequestHarness harness;
    PrivateHistoryRequestBuildRequest request = makeBuildRequest();

    const PrivateHistoryRequestBuildResult result =
        PrivateChatHistoryRequestService::buildRequest(request, harness.buildDependencies());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.pendingUpdated);
    EXPECT_TRUE(result.loadingSet);
    EXPECT_TRUE(result.dispatched);
    EXPECT_EQ(result.peerUid, kPeerUid);
    EXPECT_EQ(result.selfUid, kSelfUid);
    EXPECT_EQ(result.beforeTs, kTs2);
    EXPECT_EQ(result.beforeMsgId, QStringLiteral("before-msg-2"));

    EXPECT_EQ(harness.state.pendingPeerUid, kPeerUid);
    EXPECT_EQ(harness.state.pendingBeforeTs, kTs2);
    EXPECT_EQ(harness.state.pendingBeforeMsgId, QStringLiteral("before-msg-2"));
    EXPECT_EQ(harness.loadingCalls, 1);
    EXPECT_TRUE(harness.lastLoading);
    EXPECT_EQ(harness.dispatchCalls, 1);
    EXPECT_EQ(harness.dispatchedReqId, 1059);

    const QJsonObject payload = payloadFromBytes(harness.dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), kSelfUid);
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), kPeerUid);
    EXPECT_EQ(payload.value(QStringLiteral("before_ts")).toVariant().toLongLong(), kTs2);
    EXPECT_EQ(payload.value(QStringLiteral("before_msg_id")).toString(), QStringLiteral("before-msg-2"));
    EXPECT_EQ(payload.value(QStringLiteral("limit")).toInt(), PrivateChatHistoryRequestService::kHistoryLimit);
}

TEST(PrivateChatHistoryRequestServiceTest, ClearPendingAndClearStateResetExpectedCursorFields)
{
    PrivateHistoryRequestState state;
    state.beforeTs = 123;
    state.beforeMsgId = QStringLiteral("before");
    state.pendingPeerUid = kPeerUid;
    state.pendingBeforeTs = 456;
    state.pendingBeforeMsgId = QStringLiteral("pending");

    PrivateChatHistoryRequestService::clearPending(state);

    EXPECT_EQ(state.beforeTs, 123);
    EXPECT_EQ(state.beforeMsgId, QStringLiteral("before"));
    EXPECT_EQ(state.pendingPeerUid, 0);
    EXPECT_EQ(state.pendingBeforeTs, 0);
    EXPECT_TRUE(state.pendingBeforeMsgId.isEmpty());

    state.pendingPeerUid = kPeerUid;
    state.pendingBeforeTs = 789;
    state.pendingBeforeMsgId = QStringLiteral("pending-again");

    PrivateChatHistoryRequestService::clearState(state);

    EXPECT_EQ(state.beforeTs, 0);
    EXPECT_TRUE(state.beforeMsgId.isEmpty());
    EXPECT_EQ(state.pendingPeerUid, 0);
    EXPECT_EQ(state.pendingBeforeTs, 0);
    EXPECT_TRUE(state.pendingBeforeMsgId.isEmpty());
}

TEST(PrivateChatHistoryRequestServiceTest, BuildRequestRejectsInvalidInputsWithoutMutatingPending)
{
    {
        HistoryRequestHarness harness;
        PrivateHistoryRequestBuildRequest request = makeBuildRequest();
        request.privateHistoryLoading = true;

        const PrivateHistoryRequestBuildResult result =
            PrivateChatHistoryRequestService::buildRequest(request, harness.buildDependencies());

        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.errorText, QStringLiteral("private history is already loading"));
        EXPECT_EQ(harness.state.pendingPeerUid, 99);
        EXPECT_EQ(harness.state.pendingBeforeTs, 222);
        EXPECT_EQ(harness.state.pendingBeforeMsgId, QStringLiteral("pending-cursor"));
        EXPECT_EQ(harness.loadingCalls, 0);
        EXPECT_EQ(harness.dispatchCalls, 0);
    }

    {
        HistoryRequestHarness harness;
        PrivateHistoryRequestBuildRequest request = makeBuildRequest();
        request.peerUid = 0;

        const PrivateHistoryRequestBuildResult result =
            PrivateChatHistoryRequestService::buildRequest(request, harness.buildDependencies());

        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.errorText, QStringLiteral("invalid private peer uid"));
        EXPECT_EQ(harness.state.pendingPeerUid, 99);
        EXPECT_EQ(harness.loadingCalls, 0);
        EXPECT_EQ(harness.dispatchCalls, 0);
    }

    {
        HistoryRequestHarness harness;
        PrivateHistoryRequestBuildRequest request = makeBuildRequest();
        request.selfUid = 0;

        const PrivateHistoryRequestBuildResult result =
            PrivateChatHistoryRequestService::buildRequest(request, harness.buildDependencies());

        EXPECT_FALSE(result.success);
        EXPECT_EQ(result.errorText, QStringLiteral("current user unavailable"));
        EXPECT_EQ(harness.state.pendingPeerUid, 99);
        EXPECT_EQ(harness.loadingCalls, 0);
        EXPECT_EQ(harness.dispatchCalls, 0);
    }
}

TEST(PrivateChatHistoryRequestServiceTest, BootstrapRequestDispatchesPayloadWithoutPendingMutation)
{
    HistoryRequestHarness harness;
    PrivateHistoryBootstrapRequest request;
    request.peerUid = kPeerUid;
    request.selfUid = kSelfUid;

    PrivateHistoryBootstrapDependencies deps;
    deps.dispatchPayload = [&harness](int reqId, const QByteArray& payload)
    {
        ++harness.dispatchCalls;
        harness.dispatchedReqId = reqId;
        harness.dispatchedPayload = payload;
    };

    const PrivateHistoryRequestBuildResult result =
        PrivateChatHistoryRequestService::buildBootstrapRequest(request, deps);

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.pendingUpdated);
    EXPECT_FALSE(result.loadingSet);
    EXPECT_TRUE(result.dispatched);
    EXPECT_EQ(harness.state.pendingPeerUid, 99);
    EXPECT_EQ(harness.state.pendingBeforeTs, 222);
    EXPECT_EQ(harness.state.pendingBeforeMsgId, QStringLiteral("pending-cursor"));
    EXPECT_EQ(harness.loadingCalls, 0);
    EXPECT_EQ(harness.dispatchedReqId, 1059);

    const QJsonObject payload = payloadFromBytes(harness.dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), kSelfUid);
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), kPeerUid);
    EXPECT_EQ(payload.value(QStringLiteral("before_ts")).toVariant().toLongLong(), 0);
    EXPECT_FALSE(payload.contains(QStringLiteral("before_msg_id")));
    EXPECT_EQ(payload.value(QStringLiteral("limit")).toInt(), PrivateChatHistoryRequestService::kHistoryLimit);
}

TEST(PrivateChatHistoryRequestServiceTest, LoadCurrentClearsModelStateAndLoadingWhenNoUsablePeer)
{
    {
        HistoryRequestHarness harness;
        harness.messageModel.setMessages(
            {makeMessage(QStringLiteral("existing"), kPeerUid, kSelfUid, QStringLiteral("existing"), kTs1)}, kSelfUid);
        PrivateHistoryLoadCurrentRequest request = makeLoadCurrentRequest();
        request.currentPeerUid = 0;

        const PrivateHistoryLoadCurrentResult result =
            PrivateChatHistoryRequestService::loadCurrent(request, harness.loadCurrentDependencies());

        EXPECT_FALSE(result.success);
        EXPECT_TRUE(result.cleared);
        EXPECT_EQ(result.errorText, QStringLiteral("no current private peer"));
        EXPECT_EQ(harness.messageModel.rowCount(), 0);
        EXPECT_EQ(harness.state.beforeTs, 0);
        EXPECT_TRUE(harness.state.beforeMsgId.isEmpty());
        EXPECT_EQ(harness.state.pendingPeerUid, 0);
        EXPECT_EQ(harness.loadingCalls, 1);
        EXPECT_FALSE(harness.lastLoading);
        EXPECT_EQ(harness.canLoadCalls, 1);
        EXPECT_FALSE(harness.lastCanLoad);
    }

    {
        HistoryRequestHarness harness;
        harness.messageModel.setMessages(
            {makeMessage(QStringLiteral("existing"), kPeerUid, kSelfUid, QStringLiteral("existing"), kTs1)}, kSelfUid);
        PrivateHistoryLoadCurrentRequest request = makeLoadCurrentRequest();
        request.selfUid = 0;

        const PrivateHistoryLoadCurrentResult result =
            PrivateChatHistoryRequestService::loadCurrent(request, harness.loadCurrentDependencies());

        EXPECT_FALSE(result.success);
        EXPECT_TRUE(result.cleared);
        EXPECT_EQ(result.errorText, QStringLiteral("current user unavailable"));
        EXPECT_EQ(harness.messageModel.rowCount(), 0);
        EXPECT_EQ(harness.state.beforeTs, 0);
        EXPECT_TRUE(harness.state.beforeMsgId.isEmpty());
        EXPECT_EQ(harness.state.pendingPeerUid, 0);
        EXPECT_EQ(harness.loadingCalls, 1);
        EXPECT_FALSE(harness.lastLoading);
        EXPECT_EQ(harness.canLoadCalls, 1);
        EXPECT_FALSE(harness.lastCanLoad);
    }

    {
        HistoryRequestHarness harness;
        harness.profiles.erase(kPeerUid);
        harness.messageModel.setMessages(
            {makeMessage(QStringLiteral("existing"), kPeerUid, kSelfUid, QStringLiteral("existing"), kTs1)}, kSelfUid);

        const PrivateHistoryLoadCurrentResult result =
            PrivateChatHistoryRequestService::loadCurrent(makeLoadCurrentRequest(), harness.loadCurrentDependencies());

        EXPECT_FALSE(result.success);
        EXPECT_TRUE(result.cleared);
        EXPECT_EQ(result.errorText, QStringLiteral("private peer unavailable"));
        EXPECT_EQ(harness.messageModel.rowCount(), 0);
        EXPECT_EQ(harness.state.beforeTs, 0);
        EXPECT_TRUE(harness.state.beforeMsgId.isEmpty());
        EXPECT_EQ(harness.state.pendingPeerUid, 0);
        EXPECT_EQ(harness.loadingCalls, 1);
        EXPECT_FALSE(harness.lastLoading);
        EXPECT_EQ(harness.canLoadCalls, 1);
        EXPECT_FALSE(harness.lastCanLoad);
    }
}

TEST(PrivateChatHistoryRequestServiceTest, LoadCurrentMergesLocalCacheSetsModelCursorAndReadAck)
{
    HistoryRequestHarness harness;
    harness.localCacheMessages = {makeMessage(
        QStringLiteral("m1"), kPeerUid, kSelfUid, QStringLiteral("first peer"), kTs1),
        makeMessage(QStringLiteral("m2"), kSelfUid, kPeerUid, QStringLiteral("self reply"), kTs2),
                    makeMessage(QStringLiteral("m3"), kPeerUid, kSelfUid, QStringLiteral("latest peer"), kTs3)};

    const PrivateHistoryLoadCurrentResult result =
        PrivateChatHistoryRequestService::loadCurrent(makeLoadCurrentRequest(), harness.loadCurrentDependencies());

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.cleared);
    EXPECT_TRUE(result.shouldRequestInitialHistory);
    EXPECT_EQ(result.localCacheCount, 3);
    EXPECT_EQ(result.friendMessageCount, 3);
    EXPECT_EQ(result.beforeTs, kTs1);
    EXPECT_EQ(result.beforeMsgId, QStringLiteral("m1"));
    EXPECT_EQ(result.requestedReadAckTs, kTs3);

    EXPECT_EQ(harness.peerName, QStringLiteral("Peer"));
    EXPECT_EQ(harness.peerIcon, QStringLiteral("qrc:/res/head_1.png"));
    EXPECT_EQ(harness.loadRecentCalls, 1);
    EXPECT_EQ(harness.loadRecentOwnerUid, kSelfUid);
    EXPECT_EQ(harness.loadRecentPeerUid, kPeerUid);
    EXPECT_EQ(harness.loadRecentLimit, PrivateChatHistoryRequestService::kHistoryLimit);
    EXPECT_EQ(harness.appendCalls, 1);
    EXPECT_EQ(harness.appendedPeerUid, kPeerUid);
    EXPECT_EQ(harness.loadingCalls, 1);
    EXPECT_FALSE(harness.lastLoading);
    EXPECT_EQ(harness.canLoadCalls, 1);
    EXPECT_TRUE(harness.lastCanLoad);

    ASSERT_EQ(harness.messageModel.rowCount(), 3);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(), QStringLiteral("m1"));
    EXPECT_EQ(messageValue(harness.messageModel, 2, ChatMessageModel::MsgIdRole).toString(), QStringLiteral("m3"));
    EXPECT_EQ(harness.state.beforeTs, kTs1);
    EXPECT_EQ(harness.state.beforeMsgId, QStringLiteral("m1"));
    EXPECT_EQ(harness.state.pendingPeerUid, 0);
    EXPECT_EQ(harness.state.pendingBeforeTs, 0);
    EXPECT_TRUE(harness.state.pendingBeforeMsgId.isEmpty());
    EXPECT_EQ(harness.readAckCalls, 1);
    EXPECT_EQ(harness.readAckPeerUid, kPeerUid);
    EXPECT_EQ(harness.readAckTs, kTs3);
}

TEST(PrivateChatHistoryRequestServiceTest, LoadMorePrefersModelCursorThenFallsBackToHistoryState)
{
    HistoryRequestHarness harness;
    harness.state.beforeTs = 777;
    harness.state.beforeMsgId = QStringLiteral("state-oldest");
    harness.messageModel.setMessages(
        {makeMessage(QStringLiteral("model-oldest"), kPeerUid, kSelfUid, QStringLiteral("old"), kTs1),
                     makeMessage(QStringLiteral("model-newest"), kSelfUid, kPeerUid, QStringLiteral("new"), kTs3)},
                     kSelfUid);

    PrivateHistoryLoadMoreCursor cursor =
        PrivateChatHistoryRequestService::selectLoadMoreCursor(makeLoadMoreRequest(), harness.loadMoreDependencies());

    EXPECT_TRUE(cursor.shouldRequest);
    EXPECT_TRUE(cursor.usedModelCursor);
    EXPECT_FALSE(cursor.usedStateCursor);
    EXPECT_EQ(cursor.peerUid, kPeerUid);
    EXPECT_EQ(cursor.beforeTs, kTs1);
    EXPECT_EQ(cursor.beforeMsgId, QStringLiteral("model-oldest"));

    harness.messageModel.clear();

    cursor =
        PrivateChatHistoryRequestService::selectLoadMoreCursor(makeLoadMoreRequest(), harness.loadMoreDependencies());

    EXPECT_TRUE(cursor.shouldRequest);
    EXPECT_FALSE(cursor.usedModelCursor);
    EXPECT_TRUE(cursor.usedStateCursor);
    EXPECT_EQ(cursor.peerUid, kPeerUid);
    EXPECT_EQ(cursor.beforeTs, 777);
    EXPECT_EQ(cursor.beforeMsgId, QStringLiteral("state-oldest"));
}

TEST(PrivateChatHistoryRequestServiceTest, BuildLoadMoreRequestDispatchesPrivateHistoryRequestId)
{
    HistoryRequestHarness harness;
    harness.messageModel.setMessages(
        {makeMessage(QStringLiteral("model-oldest"), kPeerUid, kSelfUid, QStringLiteral("old"), kTs1)}, kSelfUid);

    const PrivateHistoryRequestBuildResult result =
        PrivateChatHistoryRequestService::buildLoadMoreRequest(makeLoadMoreRequest(), harness.loadMoreDependencies());

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.pendingUpdated);
    EXPECT_TRUE(result.loadingSet);
    EXPECT_TRUE(result.dispatched);
    EXPECT_EQ(harness.dispatchCalls, 1);
    EXPECT_EQ(harness.dispatchedReqId, 1059);
    EXPECT_EQ(harness.state.pendingPeerUid, kPeerUid);
    EXPECT_EQ(harness.state.pendingBeforeTs, kTs1);
    EXPECT_EQ(harness.state.pendingBeforeMsgId, QStringLiteral("model-oldest"));

    const QJsonObject payload = payloadFromBytes(harness.dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), kSelfUid);
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), kPeerUid);
    EXPECT_EQ(payload.value(QStringLiteral("before_ts")).toVariant().toLongLong(), kTs1);
    EXPECT_EQ(payload.value(QStringLiteral("before_msg_id")).toString(), QStringLiteral("model-oldest"));
    EXPECT_EQ(payload.value(QStringLiteral("limit")).toInt(), PrivateChatHistoryRequestService::kHistoryLimit);
}

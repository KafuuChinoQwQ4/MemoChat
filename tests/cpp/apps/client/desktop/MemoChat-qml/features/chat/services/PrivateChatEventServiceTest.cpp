#include "PrivateChatEventService.h"

#include "ChatMessageModel.h"
#include "FriendListModel.h"
#include "PrivateChatCacheStore.h"

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
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
constexpr qint64 kTs1 = 1700000001000LL;
constexpr qint64 kTs2 = 1700000002000LL;
constexpr qint64 kTs3 = 1700000003000LL;
constexpr qint64 kEditedAtSeconds = 1700000004LL;
constexpr qint64 kEditedAtMs = kEditedAtSeconds * 1000LL;

std::shared_ptr<FriendInfo>
makeFriend(int uid, const QString& name, const QString& lastMsg = QStringLiteral("old preview"), int unreadCount = 0)
{
    auto info = std::make_shared<FriendInfo>(
        uid,
        name,
        name,
        QStringLiteral("qrc:/res/head_1.png"), 0, QStringLiteral("desc"), name, lastMsg, QStringLiteral("u000000001"));
    info->_dialog_type = QStringLiteral("private");
    info->_unread_count = unreadCount;
    info->_last_msg_ts = 100;
    return info;
}

std::shared_ptr<TextChatData>
makeMessage(const QString& msgId,
            int fromUid,
            int toUid,
            const QString& content,
            qint64 createdAt,
            const QString& state = QStringLiteral("sent"), qint64 editedAtMs = 0, qint64 deletedAtMs = 0)
{
    return std::make_shared<TextChatData>(msgId,
                                          content,
                                          fromUid,
                                          toUid,
                                          QString(),
                                          createdAt,
                                          QString(),
                                          state,
                                          0,
                                          0,
                                          0,
                                          QString(),
                                          editedAtMs,
                                          deletedAtMs);
}

std::shared_ptr<TextChatMsg>
makeIncomingBatch(int fromUid, int toUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    auto batch = std::make_shared<TextChatMsg>(fromUid, toUid, QJsonArray{});
    batch->_chat_msgs = messages;
    return batch;
}

QVariant messageValue(const ChatMessageModel& model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

QVariantMap friendByUid(const FriendListModel& model, int uid)
{
    const int index = model.indexOfUid(uid);
    return index >= 0 ? model.get(index) : QVariantMap();
}

bool containsMessage(const std::vector<std::shared_ptr<TextChatData>>& messages, const QString& msgId)
{
    return std::any_of(messages.begin(),
                       messages.end(),
                       [&msgId](const std::shared_ptr<TextChatData>& one)
                       {
                           return one && one->_msg_id == msgId;
                       });
}

QString privateCachePath(int ownerUid)
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
           QStringLiteral("/private_chat_%1.db").arg(ownerUid);
}

struct EventHarness
{
    ChatMessageModel messageModel;
    FriendListModel chatListModel;
    FriendListModel dialogListModel;
    qint64 historyBeforeTs = 0;
    QString historyBeforeMsgId;
    std::map<int, std::shared_ptr<FriendInfo>> friends;
    PrivateChatCacheStore cacheStore;
    bool cacheEnabled = false;
    int ensureCalls = 0;
    int appendCalls = 0;
    int appendedPeerUid = 0;
    std::vector<std::shared_ptr<TextChatData>> appendedMessages;
    int updateContentCalls = 0;
    int updatedContentPeerUid = 0;
    QString updatedContentMsgId;
    int markReadCalls = 0;
    qint64 markedReadTs = 0;
    int updateStateCalls = 0;
    int updatedStatePeerUid = 0;
    QString updatedStateMsgId;
    QString updatedState;
    int readAckCalls = 0;
    int readAckPeerUid = 0;
    qint64 readAckTs = 0;

    EventHarness()
    {
        addFriend(makeFriend(kPeerUid, QStringLiteral("Peer"), QStringLiteral("old preview"), 4));
    }

    ~EventHarness()
    {
        cacheStore.close();
        if (cacheEnabled)
        {
            QFile::remove(privateCachePath(kSelfUid));
        }
    }

    bool enableCache()
    {
        QStandardPaths::setTestModeEnabled(true);
        QFile::remove(privateCachePath(kSelfUid));
        cacheEnabled = cacheStore.openForUser(kSelfUid);
        return cacheEnabled;
    }

    void addFriend(const std::shared_ptr<FriendInfo>& info)
    {
        ASSERT_NE(info, nullptr);
        friends[info->_uid] = info;
        chatListModel.upsertFriend(info);
        dialogListModel.upsertFriend(info);
    }

    void removeFriend(int uid)
    {
        friends.erase(uid);
        chatListModel.removeByUid(uid);
        dialogListModel.removeByUid(uid);
    }

    std::shared_ptr<FriendInfo> friendById(int uid) const
    {
        const auto it = friends.find(uid);
        return it == friends.end() ? nullptr : it->second;
    }

    PrivateChatEventContext context(int currentPeerUid = kPeerUid) const
    {
        PrivateChatEventContext ctx;
        ctx.selfUid = kSelfUid;
        ctx.currentPeerUid = currentPeerUid;
        ctx.currentGroupId = 0;
        return ctx;
    }

    PrivateChatEventDependencies dependencies()
    {
        PrivateChatEventDependencies deps;
        deps.cacheStore = cacheEnabled ? &cacheStore : nullptr;
        deps.messageModel = &messageModel;
        deps.chatListModel = &chatListModel;
        deps.dialogListModel = &dialogListModel;
        deps.historyBeforeTs = &historyBeforeTs;
        deps.historyBeforeMsgId = &historyBeforeMsgId;
        deps.friendById = [this](int uid)
        {
            return friendById(uid);
        };
        deps.ensureFriend = [this](int uid)
        {
            ++ensureCalls;
            auto info = makeFriend(uid, QStringLiteral("Placeholder %1").arg(uid), QString(), 0);
            addFriend(info);
            return info;
        };
        deps.appendFriendMessages = [this](int peerUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
        {
            ++appendCalls;
            appendedPeerUid = peerUid;
            appendedMessages = messages;
            const auto info = friendById(peerUid);
            if (info)
            {
                info->AppendChatMsgs(messages);
            }
        };
        deps.updatePrivateMessageContent = [this](int peerUid,
                                                  const QString& msgId,
                                                  const QString& content,
                                                  const QString& state,
                                                  qint64 editedAtMs,
                                                  qint64 deletedAtMs)
        {
            ++updateContentCalls;
            updatedContentPeerUid = peerUid;
            updatedContentMsgId = msgId;
            const auto info = friendById(peerUid);
            if (!info || msgId.isEmpty())
            {
                return false;
            }
            for (const auto& one : info->_chat_msgs)
            {
                if (!one || one->_msg_id != msgId)
                {
                    continue;
                }
                one->_msg_content = content;
                one->_msg_state = state;
                one->_edited_at_ms = editedAtMs;
                one->_deleted_at_ms = deletedAtMs;
                return true;
            }
            return false;
        };
        deps.markOutgoingReadUntil = [this](int peerUid, int selfUid, qint64 readTs)
        {
            ++markReadCalls;
            markedReadTs = readTs;
            const auto info = friendById(peerUid);
            if (!info || selfUid <= 0 || readTs <= 0)
            {
                return 0;
            }
            int updated = 0;
            for (const auto& one : info->_chat_msgs)
            {
                if (!one || one->_from_uid != selfUid || one->_created_at > readTs)
                {
                    continue;
                }
                if (one->_msg_state == QStringLiteral("sending") ||
                                                      one->_msg_state == QStringLiteral("failed") ||
                                                                                        one->_msg_state ==
                                                                                            QStringLiteral("read"))
                {
                    continue;
                }
                one->_msg_state = QStringLiteral("read");
                ++updated;
            }
            return updated;
        };
        deps.updatePrivateMessageState = [this](int peerUid, const QString& msgId, const QString& state)
        {
            ++updateStateCalls;
            updatedStatePeerUid = peerUid;
            updatedStateMsgId = msgId;
            updatedState = state;
            const auto info = friendById(peerUid);
            if (!info || msgId.isEmpty())
            {
                return false;
            }
            for (const auto& one : info->_chat_msgs)
            {
                if (!one || one->_msg_id != msgId)
                {
                    continue;
                }
                one->_msg_state = state;
                return true;
            }
            return false;
        };
        deps.requestReadAck = [this](int peerUid, qint64 ts)
        {
            ++readAckCalls;
            readAckPeerUid = peerUid;
            readAckTs = ts;
        };
        return deps;
    }
};

QJsonObject changedPayload(const QString& msgId, int fromUid, int peerUid, const QString& content)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("error"), kProtocolSuccess);
    payload.insert(QStringLiteral("fromuid"), fromUid);
    payload.insert(QStringLiteral("peer_uid"), peerUid);
    payload.insert(QStringLiteral("event"), QStringLiteral("private_msg_edited"));
    payload.insert(QStringLiteral("msgid"), msgId);
    payload.insert(QStringLiteral("content"), content);
    payload.insert(QStringLiteral("edited_at_ms"), kEditedAtSeconds);
    return payload;
}

QJsonObject revokedPayload(const QString& msgId, int fromUid, int peerUid)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("error"), kProtocolSuccess);
    payload.insert(QStringLiteral("fromuid"), fromUid);
    payload.insert(QStringLiteral("peer_uid"), peerUid);
    payload.insert(QStringLiteral("event"), QStringLiteral("private_msg_revoked"));
    payload.insert(QStringLiteral("msgid"), msgId);
    payload.insert(QStringLiteral("content"), QStringLiteral("[消息已撤回]"));
    payload.insert(QStringLiteral("deleted_at_ms"), kEditedAtSeconds);
    return payload;
}

QJsonObject readAckPayload(int readerUid, qint64 readTs)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("error"), kProtocolSuccess);
    payload.insert(QStringLiteral("fromuid"), readerUid);
    payload.insert(QStringLiteral("read_ts"), readTs);
    return payload;
}

QJsonObject statusPayload(const QString& msgId, int peerUid, const QString& status, int error = kProtocolSuccess)
{
    QJsonObject payload;
    payload.insert(QStringLiteral("error"), error);
    payload.insert(QStringLiteral("status"), status);
    payload.insert(QStringLiteral("client_msg_id"), msgId);
    payload.insert(QStringLiteral("peer_uid"), peerUid);
    return payload;
}

QJsonObject forwardPayload(const QString& clientMsgId, int peerUid, const QString& content, qint64 createdAt = kTs3)
{
    QJsonObject forwardMeta;
    forwardMeta.insert(QStringLiteral("source_msg_id"), QStringLiteral("origin-msg"));

    QJsonObject msg;
    msg.insert(QStringLiteral("msgid"), QStringLiteral("server-") + clientMsgId);
    msg.insert(QStringLiteral("content"), content);
    msg.insert(QStringLiteral("created_at"), createdAt);
    msg.insert(QStringLiteral("reply_to_server_msg_id"), 42);
    msg.insert(QStringLiteral("forward_meta"), forwardMeta);

    QJsonObject payload;
    payload.insert(QStringLiteral("peer_uid"), peerUid);
    payload.insert(QStringLiteral("client_msg_id"), clientMsgId);
    payload.insert(QStringLiteral("msg"), msg);
    return payload;
}
} // namespace

TEST(PrivateChatEventServiceTest, IncomingCurrentMessageUpdatesModelUnreadReadAckCursorAndCache)
{
    EventHarness harness;
    ASSERT_TRUE(harness.enableCache());
    const auto incoming =
        makeMessage(QStringLiteral("incoming-current"),
                                   kPeerUid,
                                   kSelfUid,
                                   QStringLiteral("edited current"), kTs2, QStringLiteral("sent"), kEditedAtMs);

    PrivateIncomingMessageRequest request;
    request.context = harness.context(kPeerUid);
    request.message = makeIncomingBatch(kPeerUid, kSelfUid, {incoming});

    const PrivateIncomingMessageResult result =
        PrivateChatEventService::handleIncomingMessage(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.peerUid, kPeerUid);
    EXPECT_FALSE(result.fromSelf);
    EXPECT_TRUE(result.currentDialog);
    EXPECT_EQ(result.messageCount, 1U);
    EXPECT_FALSE(result.ensuredFriend);
    EXPECT_TRUE(result.friendMessagesAppended);
    EXPECT_TRUE(result.cacheUpdated);
    EXPECT_TRUE(result.previewUpdated);
    EXPECT_TRUE(result.unreadCleared);
    EXPECT_FALSE(result.unreadIncremented);
    EXPECT_EQ(result.modelAppendCount, 1U);
    EXPECT_EQ(result.requestedReadAckTs, kTs2);

    EXPECT_EQ(harness.appendCalls, 1);
    EXPECT_EQ(harness.appendedPeerUid, kPeerUid);
    ASSERT_EQ(harness.appendedMessages.size(), 1U);
    EXPECT_EQ(harness.appendedMessages.front()->_msg_state, QStringLiteral("edited"));
    EXPECT_EQ(harness.readAckCalls, 1);
    EXPECT_EQ(harness.readAckPeerUid, kPeerUid);
    EXPECT_EQ(harness.readAckTs, kTs2);

    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(),
              QStringLiteral("incoming-current"));
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("edited"));
    EXPECT_EQ(harness.historyBeforeTs, kTs2);
    EXPECT_EQ(harness.historyBeforeMsgId, QStringLiteral("incoming-current"));

    const QVariantMap chatEntry = friendByUid(harness.chatListModel, kPeerUid);
    const QVariantMap dialogEntry = friendByUid(harness.dialogListModel, kPeerUid);
    EXPECT_EQ(chatEntry.value(QStringLiteral("lastMsg")).toString(), QStringLiteral("edited current"));
    EXPECT_EQ(dialogEntry.value(QStringLiteral("lastMsg")).toString(), QStringLiteral("edited current"));
    EXPECT_EQ(chatEntry.value(QStringLiteral("unreadCount")).toInt(), 0);
    EXPECT_EQ(dialogEntry.value(QStringLiteral("unreadCount")).toInt(), 0);

    const std::vector<std::shared_ptr<TextChatData>> cached =
        harness.cacheStore.loadRecentMessages(kSelfUid, kPeerUid, 20);
    EXPECT_TRUE(containsMessage(cached, QStringLiteral("incoming-current")));
}

TEST(PrivateChatEventServiceTest, IncomingBackgroundMessageEnsuresFriendAndIncrementsUnreadWithoutCurrentModelMutation)
{
    EventHarness harness;
    harness.messageModel.setMessages(
        {makeMessage(QStringLiteral("current"), kPeerUid, kSelfUid, QStringLiteral("current"), kTs1)}, kSelfUid);
    harness.removeFriend(kOtherPeerUid);
    const auto background =
        makeMessage(QStringLiteral("background"), kOtherPeerUid, kSelfUid, QStringLiteral("background text"), kTs3);

    PrivateIncomingMessageRequest request;
    request.context = harness.context(kPeerUid);
    request.message = makeIncomingBatch(kOtherPeerUid, kSelfUid, {background});

    const PrivateIncomingMessageResult result =
        PrivateChatEventService::handleIncomingMessage(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.peerUid, kOtherPeerUid);
    EXPECT_TRUE(result.ensuredFriend);
    EXPECT_EQ(harness.ensureCalls, 1);
    EXPECT_TRUE(result.friendMessagesAppended);
    EXPECT_FALSE(result.cacheUpdated);
    EXPECT_TRUE(result.previewUpdated);
    EXPECT_FALSE(result.currentDialog);
    EXPECT_FALSE(result.unreadCleared);
    EXPECT_TRUE(result.unreadIncremented);
    EXPECT_EQ(result.modelAppendCount, 0U);
    EXPECT_EQ(result.requestedReadAckTs, 0);
    EXPECT_EQ(harness.readAckCalls, 0);

    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(), QStringLiteral("current"));
    const QVariantMap dialogEntry = friendByUid(harness.dialogListModel, kOtherPeerUid);
    EXPECT_EQ(dialogEntry.value(QStringLiteral("lastMsg")).toString(), QStringLiteral("background text"));
    EXPECT_EQ(dialogEntry.value(QStringLiteral("unreadCount")).toInt(), 1);
}

TEST(PrivateChatEventServiceTest, MessageChangedPatchesCurrentModelPreviewAndCache)
{
    EventHarness harness;
    ASSERT_TRUE(harness.enableCache());
    const auto original =
        makeMessage(QStringLiteral("changed-current"), kPeerUid, kSelfUid, QStringLiteral("before"), kTs1);
    harness.friendById(kPeerUid)->AppendChatMsgs({original});
    harness.messageModel.setMessages(harness.friendById(kPeerUid)->_chat_msgs, kSelfUid);

    PrivateMessageChangedRequest request;
    request.context = harness.context(kPeerUid);
    request.payload =
        changedPayload(QStringLiteral("changed-current"), kPeerUid, kPeerUid, QStringLiteral("after edit"));

    const PrivateMessageChangedResult result =
        PrivateChatEventService::handleMessageChanged(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.peerUid, kPeerUid);
    EXPECT_EQ(result.msgId, QStringLiteral("changed-current"));
    EXPECT_EQ(result.content, QStringLiteral("after edit"));
    EXPECT_EQ(result.state, QStringLiteral("edited"));
    EXPECT_EQ(result.editedAtMs, kEditedAtMs);
    EXPECT_TRUE(result.userManagerUpdated);
    EXPECT_TRUE(result.cacheUpdated);
    EXPECT_TRUE(result.previewUpdated);
    EXPECT_TRUE(result.modelPatched);
    EXPECT_EQ(harness.updateContentCalls, 1);
    EXPECT_EQ(harness.updatedContentPeerUid, kPeerUid);

    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::RawContentRole).toString(),
              QStringLiteral("after edit"));
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("edited"));
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::EditedAtMsRole).toLongLong(), kEditedAtMs);
    EXPECT_EQ(friendByUid(harness.dialogListModel, kPeerUid)
                  .value(QStringLiteral("lastMsg")).toString(), QStringLiteral("after edit"));

    const std::vector<std::shared_ptr<TextChatData>> cached =
        harness.cacheStore.loadRecentMessages(kSelfUid, kPeerUid, 20);
    EXPECT_TRUE(containsMessage(cached, QStringLiteral("changed-current")));
}

TEST(PrivateChatEventServiceTest, MessageChangedRevokesCurrentModelPreviewAndCache)
{
    EventHarness harness;
    ASSERT_TRUE(harness.enableCache());
    const auto original =
        makeMessage(QStringLiteral("revoked-current"), kPeerUid, kSelfUid, QStringLiteral("before revoke"), kTs1);
    harness.friendById(kPeerUid)->AppendChatMsgs({original});
    harness.messageModel.setMessages(harness.friendById(kPeerUid)->_chat_msgs, kSelfUid);

    PrivateMessageChangedRequest request;
    request.context = harness.context(kPeerUid);
    request.payload = revokedPayload(QStringLiteral("revoked-current"), kPeerUid, kPeerUid);

    const PrivateMessageChangedResult result =
        PrivateChatEventService::handleMessageChanged(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.peerUid, kPeerUid);
    EXPECT_EQ(result.msgId, QStringLiteral("revoked-current"));
    EXPECT_EQ(result.content, QStringLiteral("[消息已撤回]"));
    EXPECT_EQ(result.state, QStringLiteral("deleted"));
    EXPECT_EQ(result.deletedAtMs, kEditedAtMs);
    EXPECT_TRUE(result.userManagerUpdated);
    EXPECT_TRUE(result.cacheUpdated);
    EXPECT_TRUE(result.previewUpdated);
    EXPECT_TRUE(result.modelPatched);

    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::RawContentRole).toString(),
              QStringLiteral("[消息已撤回]"));
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("deleted"));
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::DeletedAtMsRole).toLongLong(), kEditedAtMs);

    const std::vector<std::shared_ptr<TextChatData>> cached =
        harness.cacheStore.loadRecentMessages(kSelfUid, kPeerUid, 20);
    EXPECT_TRUE(containsMessage(cached, QStringLiteral("revoked-current")));
}

TEST(PrivateChatEventServiceTest, MessageChangedForBackgroundPeerSkipsCurrentModelPatch)
{
    EventHarness harness;
    harness.addFriend(makeFriend(kOtherPeerUid, QStringLiteral("Other"), QStringLiteral("old other"), 2));
    harness.friendById(kPeerUid)->AppendChatMsgs(
        {makeMessage(QStringLiteral("current"), kPeerUid, kSelfUid, QStringLiteral("current"), kTs1)});
    harness.friendById(kOtherPeerUid)
        ->AppendChatMsgs({makeMessage(
            QStringLiteral("changed-background"), kOtherPeerUid, kSelfUid, QStringLiteral("background before"), kTs2)});
    harness.messageModel.setMessages(harness.friendById(kPeerUid)->_chat_msgs, kSelfUid);

    PrivateMessageChangedRequest request;
    request.context = harness.context(kPeerUid);
    request.payload = changedPayload(
        QStringLiteral("changed-background"), kOtherPeerUid, kOtherPeerUid, QStringLiteral("background after"));

    const PrivateMessageChangedResult result =
        PrivateChatEventService::handleMessageChanged(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.peerUid, kOtherPeerUid);
    EXPECT_TRUE(result.userManagerUpdated);
    EXPECT_FALSE(result.cacheUpdated);
    EXPECT_TRUE(result.previewUpdated);
    EXPECT_FALSE(result.modelPatched);
    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(), QStringLiteral("current"));
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::RawContentRole).toString(),
              QStringLiteral("current"));
    EXPECT_EQ(friendByUid(harness.dialogListModel, kOtherPeerUid)
                  .value(QStringLiteral("lastMsg")).toString(), QStringLiteral("background after"));
}

TEST(PrivateChatEventServiceTest, ForwardResponseAppendsCachePreviewAndCurrentModel)
{
    EventHarness harness;
    ASSERT_TRUE(harness.enableCache());

    PrivateForwardResponseRequest request;
    request.context = harness.context(kPeerUid);
    request.payload =
        forwardPayload(QStringLiteral("forward-current"), kPeerUid, QStringLiteral("forwarded text"), kTs2 / 1000);

    const PrivateForwardResponseResult result =
        PrivateChatEventService::handleForwardResponse(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.isError);
    EXPECT_EQ(result.statusText, QStringLiteral("消息已转发"));
    EXPECT_EQ(result.peerUid, kPeerUid);
    EXPECT_EQ(result.msgId, QStringLiteral("forward-current"));
    EXPECT_EQ(result.content, QStringLiteral("forwarded text"));
    EXPECT_EQ(result.createdAt, kTs2);
    EXPECT_TRUE(result.userManagerUpdated);
    EXPECT_TRUE(result.cacheUpdated);
    EXPECT_TRUE(result.previewUpdated);
    EXPECT_TRUE(result.modelUpserted);
    EXPECT_EQ(harness.appendCalls, 1);
    EXPECT_EQ(harness.appendedPeerUid, kPeerUid);
    ASSERT_EQ(harness.appendedMessages.size(), 1U);
    EXPECT_EQ(harness.appendedMessages.front()->_from_uid, kSelfUid);
    EXPECT_EQ(harness.appendedMessages.front()->_to_uid, kPeerUid);
    EXPECT_EQ(harness.appendedMessages.front()->_reply_to_server_msg_id, 42);
    EXPECT_FALSE(harness.appendedMessages.front()->_forward_meta_json.isEmpty());

    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(),
              QStringLiteral("forward-current"));
    EXPECT_TRUE(messageValue(harness.messageModel, 0, ChatMessageModel::OutgoingRole).toBool());
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::RawContentRole).toString(),
              QStringLiteral("forwarded text"));
    EXPECT_EQ(friendByUid(harness.dialogListModel, kPeerUid)
                  .value(QStringLiteral("lastMsg")).toString(), QStringLiteral("forwarded text"));

    const std::vector<std::shared_ptr<TextChatData>> cached =
        harness.cacheStore.loadRecentMessages(kSelfUid, kPeerUid, 20);
    EXPECT_TRUE(containsMessage(cached, QStringLiteral("forward-current")));
}

TEST(PrivateChatEventServiceTest, ForwardResponseForBackgroundPeerSkipsCurrentModel)
{
    EventHarness harness;
    harness.addFriend(makeFriend(kOtherPeerUid, QStringLiteral("Other"), QStringLiteral("old other"), 2));
    harness.messageModel.setMessages(
        {makeMessage(QStringLiteral("current"), kPeerUid, kSelfUid, QStringLiteral("current"), kTs1)}, kSelfUid);

    PrivateForwardResponseRequest request;
    request.context = harness.context(kPeerUid);
    request.payload =
        forwardPayload(QStringLiteral("forward-background"), kOtherPeerUid, QStringLiteral("background forward"), kTs3);

    const PrivateForwardResponseResult result =
        PrivateChatEventService::handleForwardResponse(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.peerUid, kOtherPeerUid);
    EXPECT_TRUE(result.userManagerUpdated);
    EXPECT_FALSE(result.cacheUpdated);
    EXPECT_TRUE(result.previewUpdated);
    EXPECT_FALSE(result.modelUpserted);
    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(), QStringLiteral("current"));
    EXPECT_EQ(friendByUid(harness.dialogListModel, kOtherPeerUid)
                  .value(QStringLiteral("lastMsg")).toString(), QStringLiteral("background forward"));
}

TEST(PrivateChatEventServiceTest, ForwardResponseRejectsMissingPeerOrMessageWithoutSideEffects)
{
    EventHarness harness;

    PrivateForwardResponseRequest missingPeer;
    missingPeer.context = harness.context(kPeerUid);
    missingPeer.payload = forwardPayload(QStringLiteral("forward-missing-peer"), 0, QStringLiteral("text"), kTs2);

    const PrivateForwardResponseResult missingPeerResult =
        PrivateChatEventService::handleForwardResponse(missingPeer, harness.dependencies());

    EXPECT_FALSE(missingPeerResult.success);
    EXPECT_TRUE(missingPeerResult.isError);
    EXPECT_EQ(missingPeerResult.statusText, QStringLiteral("消息转发失败"));
    EXPECT_EQ(harness.appendCalls, 0);
    EXPECT_EQ(harness.messageModel.rowCount(), 0);

    PrivateForwardResponseRequest missingMessage;
    missingMessage.context = harness.context(kPeerUid);
    missingMessage.payload.insert(QStringLiteral("peer_uid"), kPeerUid);

    const PrivateForwardResponseResult missingMessageResult =
        PrivateChatEventService::handleForwardResponse(missingMessage, harness.dependencies());

    EXPECT_FALSE(missingMessageResult.success);
    EXPECT_TRUE(missingMessageResult.isError);
    EXPECT_EQ(harness.appendCalls, 0);
    EXPECT_EQ(harness.messageModel.rowCount(), 0);
}

TEST(PrivateChatEventServiceTest, ForwardResponseRejectsEmptyMessageIdOrContentWithoutSideEffects)
{
    EventHarness harness;

    PrivateForwardResponseRequest emptyMsgId;
    emptyMsgId.context = harness.context(kPeerUid);
    emptyMsgId.payload = forwardPayload(QString(), kPeerUid, QStringLiteral("text"), kTs2);
    QJsonObject msgWithoutServerId = emptyMsgId.payload.value(QStringLiteral("msg")).toObject();
    msgWithoutServerId.remove(QStringLiteral("msgid"));
    emptyMsgId.payload.insert(QStringLiteral("msg"), msgWithoutServerId);

    const PrivateForwardResponseResult emptyMsgIdResult =
        PrivateChatEventService::handleForwardResponse(emptyMsgId, harness.dependencies());

    EXPECT_FALSE(emptyMsgIdResult.success);
    EXPECT_TRUE(emptyMsgIdResult.isError);
    EXPECT_EQ(harness.appendCalls, 0);
    EXPECT_EQ(harness.messageModel.rowCount(), 0);

    PrivateForwardResponseRequest emptyContent;
    emptyContent.context = harness.context(kPeerUid);
    emptyContent.payload = forwardPayload(QStringLiteral("forward-empty-content"), kPeerUid, QString(), kTs2);

    const PrivateForwardResponseResult emptyContentResult =
        PrivateChatEventService::handleForwardResponse(emptyContent, harness.dependencies());

    EXPECT_FALSE(emptyContentResult.success);
    EXPECT_TRUE(emptyContentResult.isError);
    EXPECT_EQ(harness.appendCalls, 0);
    EXPECT_EQ(harness.messageModel.rowCount(), 0);
}

TEST(PrivateChatEventServiceTest, ReadAckNormalizesSecondsAndPatchesOnlyReadSelfSentCurrentMessages)
{
    EventHarness harness;
    ASSERT_TRUE(harness.enableCache());
    harness.friendById(kPeerUid)->AppendChatMsgs({makeMessage(
        QStringLiteral("self-old"), kSelfUid, kPeerUid, QStringLiteral("old self"), kTs1),
        makeMessage(QStringLiteral("peer-mid"), kPeerUid, kSelfUid, QStringLiteral("peer"), kTs2),
                    makeMessage(QStringLiteral("self-new"), kSelfUid, kPeerUid, QStringLiteral("new self"), kTs3)});
    harness.messageModel.setMessages(harness.friendById(kPeerUid)->_chat_msgs, kSelfUid);

    PrivateReadAckRequest request;
    request.context = harness.context(kPeerUid);
    request.payload = readAckPayload(kPeerUid, kTs2 / 1000);

    const PrivateReadAckResult result = PrivateChatEventService::handleReadAck(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.peerUid, kPeerUid);
    EXPECT_EQ(result.readTs, kTs2);
    EXPECT_EQ(result.updatedCount, 1);
    EXPECT_TRUE(result.cacheUpdated);
    EXPECT_EQ(result.modelPatchCount, 1);
    EXPECT_EQ(harness.markReadCalls, 1);
    EXPECT_EQ(harness.markedReadTs, kTs2);

    ASSERT_EQ(harness.messageModel.rowCount(), 3);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MsgIdRole).toString(),
              QStringLiteral("self-old"));
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("read"));
    EXPECT_EQ(messageValue(harness.messageModel, 1, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("sent"));
    EXPECT_EQ(messageValue(harness.messageModel, 2, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("sent"));
}

TEST(PrivateChatEventServiceTest, BuildReadAckPayloadUsesChatOwnedPrivateSchema)
{
    const QByteArray payload = PrivateChatEventService::buildReadAckPayload(kSelfUid, kPeerUid, kTs2);

    const QJsonObject obj = QJsonDocument::fromJson(payload).object();
    EXPECT_EQ(obj.value(QStringLiteral("fromuid")).toInt(), kSelfUid);
    EXPECT_EQ(obj.value(QStringLiteral("peer_uid")).toInt(), kPeerUid);
    EXPECT_EQ(obj.value(QStringLiteral("read_ts")).toVariant().toLongLong(), kTs2);
}

TEST(PrivateChatEventServiceTest, BuildReadAckPayloadRejectsMissingParticipants)
{
    EXPECT_TRUE(PrivateChatEventService::buildReadAckPayload(0, kPeerUid, kTs2).isEmpty());
    EXPECT_TRUE(PrivateChatEventService::buildReadAckPayload(kSelfUid, 0, kTs2).isEmpty());
}

TEST(PrivateChatEventServiceTest, MessageStatusUpdatesCurrentModelAndCacheForPrivateScope)
{
    EventHarness harness;
    ASSERT_TRUE(harness.enableCache());
    harness.friendById(kPeerUid)->AppendChatMsgs(
        {makeMessage(QStringLiteral("status-current"),
                                    kSelfUid,
                                    kPeerUid,
                                    QStringLiteral("optimistic"), kTs1, QStringLiteral("sending"))});
    harness.messageModel.setMessages(harness.friendById(kPeerUid)->_chat_msgs, kSelfUid);

    PrivateMessageStatusRequest request;
    request.context = harness.context(kPeerUid);
    request.payload = statusPayload(QStringLiteral("status-current"), kPeerUid, QStringLiteral("persisted"));

    const PrivateMessageStatusResult result =
        PrivateChatEventService::handleMessageStatus(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.peerUid, kPeerUid);
    EXPECT_EQ(result.clientMsgId, QStringLiteral("status-current"));
    EXPECT_EQ(result.nextState, QStringLiteral("sent"));
    EXPECT_TRUE(result.userManagerUpdated);
    EXPECT_TRUE(result.cacheUpdated);
    EXPECT_TRUE(result.modelPatched);
    EXPECT_EQ(harness.updateStateCalls, 1);
    EXPECT_EQ(harness.updatedStatePeerUid, kPeerUid);
    EXPECT_EQ(harness.updatedStateMsgId, QStringLiteral("status-current"));
    EXPECT_EQ(harness.updatedState, QStringLiteral("sent"));
    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("sent"));
}

TEST(PrivateChatEventServiceTest, MessageStatusFailureUpdatesCurrentModelAndCache)
{
    EventHarness harness;
    ASSERT_TRUE(harness.enableCache());
    harness.friendById(kPeerUid)->AppendChatMsgs(
        {makeMessage(QStringLiteral("status-failed"),
                                    kSelfUid,
                                    kPeerUid,
                                    QStringLiteral("optimistic"), kTs1, QStringLiteral("sending"))});
    harness.messageModel.setMessages(harness.friendById(kPeerUid)->_chat_msgs, kSelfUid);

    PrivateMessageStatusRequest request;
    request.context = harness.context(kPeerUid);
    request.payload = statusPayload(QStringLiteral("status-failed"), kPeerUid, QString(), kProtocolJsonError);

    const PrivateMessageStatusResult result =
        PrivateChatEventService::handleMessageStatus(request, harness.dependencies());

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.peerUid, kPeerUid);
    EXPECT_EQ(result.clientMsgId, QStringLiteral("status-failed"));
    EXPECT_EQ(result.nextState, QStringLiteral("failed"));
    EXPECT_TRUE(result.userManagerUpdated);
    EXPECT_TRUE(result.cacheUpdated);
    EXPECT_TRUE(result.modelPatched);
    EXPECT_EQ(harness.updateStateCalls, 1);
    EXPECT_EQ(harness.updatedStatePeerUid, kPeerUid);
    EXPECT_EQ(harness.updatedStateMsgId, QStringLiteral("status-failed"));
    EXPECT_EQ(harness.updatedState, QStringLiteral("failed"));
    ASSERT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("failed"));
}

TEST(PrivateChatEventServiceTest, PrivateMutationResponseOwnsEventAndStatusPolicy)
{
    EventHarness harness;
    harness.friendById(kPeerUid)->AppendChatMsgs(
        {makeMessage(QStringLiteral("edit-response"), kSelfUid, kPeerUid, QStringLiteral("old"), kTs1)});
    harness.messageModel.setMessages(harness.friendById(kPeerUid)->_chat_msgs, kSelfUid);

    QJsonObject editPayload;
    editPayload.insert(QStringLiteral("fromuid"), kSelfUid);
    editPayload.insert(QStringLiteral("peer_uid"), kPeerUid);
    editPayload.insert(QStringLiteral("msgid"), QStringLiteral("edit-response"));
    editPayload.insert(QStringLiteral("content"), QStringLiteral("new"));
    editPayload.insert(QStringLiteral("edited_at_ms"), kEditedAtSeconds);

    PrivateMessageMutationResponseRequest editRequest;
    editRequest.context = harness.context(kPeerUid);
    editRequest.reqId = 1078;
    editRequest.payload = editPayload;

    const PrivateMessageChangedResult editResult =
        PrivateChatEventService::handleMessageMutationResponse(editRequest, harness.dependencies());

    EXPECT_TRUE(editResult.success);
    EXPECT_EQ(editResult.event, QStringLiteral("private_msg_edited"));
    EXPECT_EQ(editResult.statusText, QStringLiteral("私聊消息已编辑"));
    EXPECT_FALSE(editResult.statusIsError);
    EXPECT_EQ(harness.updatedContentMsgId, QStringLiteral("edit-response"));
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::ContentRole).toString(), QStringLiteral("new"));
    EXPECT_EQ(messageValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("edited"));

    harness.friendById(kPeerUid)->AppendChatMsgs(
        {makeMessage(QStringLiteral("revoke-response"), kSelfUid, kPeerUid, QStringLiteral("old"), kTs2)});

    QJsonObject revokePayload;
    revokePayload.insert(QStringLiteral("fromuid"), kSelfUid);
    revokePayload.insert(QStringLiteral("peer_uid"), kPeerUid);
    revokePayload.insert(QStringLiteral("msgid"), QStringLiteral("revoke-response"));
    revokePayload.insert(QStringLiteral("content"), QStringLiteral("[消息已撤回]"));
    revokePayload.insert(QStringLiteral("deleted_at_ms"), kEditedAtSeconds);

    PrivateMessageMutationResponseRequest revokeRequest;
    revokeRequest.context = harness.context(kPeerUid);
    revokeRequest.reqId = 1080;
    revokeRequest.payload = revokePayload;

    const PrivateMessageChangedResult revokeResult =
        PrivateChatEventService::handleMessageMutationResponse(revokeRequest, harness.dependencies());

    EXPECT_TRUE(revokeResult.success);
    EXPECT_EQ(revokeResult.event, QStringLiteral("private_msg_revoked"));
    EXPECT_EQ(revokeResult.statusText, QStringLiteral("私聊消息已撤回"));
    EXPECT_FALSE(revokeResult.statusIsError);
    EXPECT_EQ(harness.updatedContentMsgId, QStringLiteral("revoke-response"));
}

TEST(PrivateChatEventServiceTest, ResolveMessageStatusStateKeepsLegacyTransitions)
{
    EXPECT_EQ(PrivateChatEventService::resolveMessageStatusState(
        statusPayload(QStringLiteral("m1"), kPeerUid, QStringLiteral("persisted"))),
        QStringLiteral("sent"));
    EXPECT_EQ(PrivateChatEventService::resolveMessageStatusState(
        statusPayload(QStringLiteral("m1"), kPeerUid, QStringLiteral("delivered"))),
        QStringLiteral("sent"));
    EXPECT_EQ(PrivateChatEventService::resolveMessageStatusState(
        statusPayload(QStringLiteral("m1"), kPeerUid, QStringLiteral("accepted"))),
        QStringLiteral("accepted"));
    EXPECT_EQ(PrivateChatEventService::resolveMessageStatusState(
        statusPayload(QStringLiteral("m1"), kPeerUid, QStringLiteral("queued_retry"))),
        QStringLiteral("queued_retry"));
    EXPECT_EQ(PrivateChatEventService::resolveMessageStatusState(
        statusPayload(QStringLiteral("m1"), kPeerUid, QStringLiteral("offline_pending"))),
        QStringLiteral("offline_pending"));
    EXPECT_EQ(PrivateChatEventService::resolveMessageStatusState(
        statusPayload(QStringLiteral("m1"), kPeerUid, QStringLiteral("custom_state"))),
        QStringLiteral("custom_state"));
    EXPECT_EQ(PrivateChatEventService::resolveMessageStatusState(
        statusPayload(QStringLiteral("m1"), kPeerUid, QString(), kProtocolJsonError)),
        QStringLiteral("failed"));
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

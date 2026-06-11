#include "ChatFeatureController.h"
#include "ChatMessageModel.h"
#include "ConversationSyncService.h"
#include "FriendListModel.h"
#include "PrivateChatEventService.h"
#include "PrivateChatHistoryRequestService.h"
#include "PrivateChatSendService.h"

#include <gtest/gtest.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QVariantMap>
#include <QVector>
#include <utility>
#include <vector>

namespace
{
struct HandlerCalls
{
    int ensureChatListInitialized = 0;
    int ensureGroupsInitialized = 0;
    int selectDialogByUid = 0;
    int selectChatIndex = 0;
    int selectGroupIndex = 0;
    int loadMoreChats = 0;
    int loadMorePrivateHistory = 0;
    int sendCurrentComposerPayload = 0;
    int sendImageMessage = 0;
    int sendFileMessage = 0;
    int removePendingAttachment = 0;
    int clearPendingAttachments = 0;
    int syncCurrentDraftText = 0;
    int syncCurrentDialogPinned = 0;
    int syncCurrentDialogMuted = 0;
    int startVoiceChat = 0;
    int startVideoChat = 0;

    int selectedDialogUid = 0;
    int selectedChatIndex = 0;
    int selectedGroupIndex = 0;
    QString sentComposerText;
    QString removedAttachmentId;
    QString syncedDraftText;

    int totalCalls() const
    {
        return ensureChatListInitialized + ensureGroupsInitialized + selectDialogByUid + selectChatIndex +
               selectGroupIndex + loadMoreChats + loadMorePrivateHistory + sendCurrentComposerPayload +
               sendImageMessage + sendFileMessage + removePendingAttachment + clearPendingAttachments +
               syncCurrentDraftText + syncCurrentDialogPinned + syncCurrentDialogMuted + startVoiceChat +
               startVideoChat;
    }
};

void configureCountingPorts(ChatFeatureController& controller, HandlerCalls& calls)
{
    ChatDialogNavigationPort dialogNavigationPort;
    dialogNavigationPort.ensureChatListInitialized = [&calls]()
    {
        ++calls.ensureChatListInitialized;
    };
    dialogNavigationPort.selectDialogByUid = [&calls](int uid)
    {
        ++calls.selectDialogByUid;
        calls.selectedDialogUid = uid;
    };
    dialogNavigationPort.selectChatIndex = [&calls](int index)
    {
        ++calls.selectChatIndex;
        calls.selectedChatIndex = index;
    };
    dialogNavigationPort.loadMoreChats = [&calls]()
    {
        ++calls.loadMoreChats;
    };

    ChatCurrentHistoryPort currentHistoryPort;
    currentHistoryPort.snapshot = [&calls]()
    {
        ++calls.loadMorePrivateHistory;
        ChatHistoryLoadMoreRequest request;
        request.privateHistoryLoading = true;
        return request;
    };

    ChatGroupConversationPort groupConversationPort;
    groupConversationPort.ensureGroupsInitialized = [&calls]()
    {
        ++calls.ensureGroupsInitialized;
    };
    groupConversationPort.selectGroupIndex = [&calls](int index)
    {
        ++calls.selectGroupIndex;
        calls.selectedGroupIndex = index;
    };

    ChatPrivateCommandPort privateCommandPort;
    privateCommandPort.sendCurrentComposerPayload = [&calls](const QString& text)
    {
        ++calls.sendCurrentComposerPayload;
        calls.sentComposerText = text;
    };
    privateCommandPort.sendImageMessage = [&calls]()
    {
        ++calls.sendImageMessage;
    };
    privateCommandPort.sendFileMessage = [&calls]()
    {
        ++calls.sendFileMessage;
    };

    ChatPendingAttachmentPort pendingAttachmentPort;
    pendingAttachmentPort.removePendingAttachment = [&calls](const QString& attachmentId)
    {
        ++calls.removePendingAttachment;
        calls.removedAttachmentId = attachmentId;
    };
    pendingAttachmentPort.clearPendingAttachments = [&calls]()
    {
        ++calls.clearPendingAttachments;
    };

    ChatDialogRuntimePort dialogRuntimePort;
    dialogRuntimePort.syncCurrentDraftText = [&calls](const QString& text)
    {
        ++calls.syncCurrentDraftText;
        calls.syncedDraftText = text;
    };
    dialogRuntimePort.syncCurrentDialogPinned = [&calls](bool)
    {
        ++calls.syncCurrentDialogPinned;
    };
    dialogRuntimePort.syncCurrentDialogMuted = [&calls](bool)
    {
        ++calls.syncCurrentDialogMuted;
    };
    ChatShellIntentPort shellIntentPort;
    shellIntentPort.startVoiceChat = [&calls]()
    {
        ++calls.startVoiceChat;
    };
    shellIntentPort.startVideoChat = [&calls]()
    {
        ++calls.startVideoChat;
    };

    controller.setDialogNavigationPort(std::move(dialogNavigationPort));
    controller.setGroupConversationPort(std::move(groupConversationPort));
    controller.setPrivateCommandPort(std::move(privateCommandPort));
    controller.setCurrentHistoryPort(std::move(currentHistoryPort));
    controller.setPendingAttachmentPort(std::move(pendingAttachmentPort));
    controller.setDialogRuntimePort(std::move(dialogRuntimePort));
    controller.setShellIntentPort(std::move(shellIntentPort));
}

struct GroupConversationHarness
{
    GroupConversationState state;
    QMap<int, qint64> dialogUidMap;
    ChatMessageModel messageModel;
    FriendListModel groupListModel;
    FriendListModel dialogListModel;
    QHash<qint64, std::shared_ptr<GroupInfoData>> groups;
    QHash<int, int> mentionMap;
    QByteArray lastChatPayload;
    QByteArray lastHistoryPayload;
    QByteArray lastReadAckPayload;
    int lastChatRequestId = 0;
    int lastHistoryRequestId = 0;
    int chatDispatchCount = 0;
    int historyDispatchCount = 0;
    int readAckDispatchCount = 0;
    int outgoingReadUpdateCount = 0;
    qint64 outgoingReadGroupId = 0;
    int outgoingReadOwnerUid = 0;
    qint64 outgoingReadTs = 0;

    GroupConversationHarness()
    {
        seedGroup(7001, QStringLiteral("Team"));
    }

    int dialogUidFor(qint64 groupId)
    {
        return ConversationSyncService::resolveGroupDialogUid(dialogUidMap, groupId);
    }

    std::shared_ptr<GroupInfoData> seedGroup(qint64 groupId, const QString& name)
    {
        const int dialogUid = dialogUidFor(groupId);
        auto group = std::make_shared<GroupInfoData>();
        group->_group_id = groupId;
        group->_name = name;
        group->_icon = QStringLiteral("qrc:/res/chat_icon.png");
        groups.insert(groupId, group);
        auto item = std::make_shared<FriendInfo>(dialogUid,
                                                 name,
                                                 name,
                                                 group->_icon,
                                                 0,
                                                 QString(),
                                                 QString(),
                                                 QString(),
                                                 QString());
        item->_dialog_type = QStringLiteral("group");
        groupListModel.upsertFriend(item);
        dialogListModel.upsertFriend(item);
        return group;
    }

    GroupConversationContext context(qint64 currentGroupId = 7001, int currentPrivatePeerUid = 0) const
    {
        GroupConversationContext value;
        value.selfUid = 1001;
        value.selfName = QStringLiteral("Me");
        value.selfIcon = QStringLiteral("me.png");
        value.currentGroupId = currentGroupId;
        value.currentPrivatePeerUid = currentPrivatePeerUid;
        return value;
    }

    GroupConversationDependencies dependencies()
    {
        GroupConversationDependencies deps;
        deps.state = &state;
        deps.dialogUidMap = &dialogUidMap;
        deps.messageModel = &messageModel;
        deps.groupListModel = &groupListModel;
        deps.dialogListModel = &dialogListModel;
        deps.groupById = [this](qint64 groupId)
        {
            return groups.value(groupId);
        };
        deps.upsertGroup = [this](std::shared_ptr<GroupInfoData> group)
        {
            if (group)
            {
                groups.insert(group->_group_id, group);
            }
        };
        deps.upsertGroupMessage = [this](qint64 groupId, const std::shared_ptr<TextChatData>& message)
        {
            auto group = groups.value(groupId);
            if (!group)
            {
                group = seedGroup(groupId, QStringLiteral("Team%1").arg(groupId));
            }
            for (auto& existing : group->_chat_msgs)
            {
                if (existing && message && existing->_msg_id == message->_msg_id)
                {
                    existing = message;
                    return;
                }
            }
            group->_chat_msgs.push_back(message);
        };
        deps.updateGroupMessageState = [this](qint64 groupId, const QString& msgId, const QString& stateText)
        {
            const auto group = groups.value(groupId);
            if (!group)
            {
                return false;
            }
            for (const auto& message : group->_chat_msgs)
            {
                if (message && message->_msg_id == msgId)
                {
                    message->_msg_state = stateText;
                    return true;
                }
            }
            return false;
        };
        deps.updateGroupMessageContent = [this](qint64 groupId,
                                                const QString& msgId,
                                                const QString& content,
                                                const QString& stateText,
                                                qint64 editedAtMs,
                                                qint64 deletedAtMs)
        {
            const auto group = groups.value(groupId);
            if (!group)
            {
                return false;
            }
            for (const auto& message : group->_chat_msgs)
            {
                if (message && message->_msg_id == msgId)
                {
                    message->_msg_content = content;
                    message->_msg_state = stateText;
                    message->_edited_at_ms = editedAtMs;
                    message->_deleted_at_ms = deletedAtMs;
                    return true;
                }
            }
            return false;
        };
        deps.markGroupOutgoingReadUntil = [this](qint64 groupId, int ownerUid, qint64 readTs)
        {
            outgoingReadGroupId = groupId;
            outgoingReadOwnerUid = ownerUid;
            outgoingReadTs = readTs;
            ++outgoingReadUpdateCount;
            return 1;
        };
        deps.dispatchPayload = [this](int reqId, const QByteArray& payload)
        {
            if (reqId == 1047)
            {
                lastHistoryRequestId = reqId;
                lastHistoryPayload = payload;
                ++historyDispatchCount;
                return;
            }
            lastChatRequestId = reqId;
            lastChatPayload = payload;
            ++chatDispatchCount;
        };
        deps.clearGroupUnreadAndMention = [this](FriendListModel& model, int dialogUid)
        {
            ConversationSyncService::clearGroupUnreadAndMention(model, mentionMap, dialogUid);
        };
        deps.incrementGroupUnreadAndMention = [this](FriendListModel& model, int dialogUid, bool mentioned)
        {
            ConversationSyncService::incrementGroupUnreadAndMention(model, mentionMap, dialogUid, mentioned);
        };
        deps.dialogDecorationState = []
        {
            return DialogDecorationState();
        };
        return deps;
    }
};

QJsonObject compactJsonObject(const QByteArray& payload)
{
    return QJsonDocument::fromJson(payload).object();
}

QVariant modelValue(const ChatMessageModel& model, int row, int role)
{
    return model.data(model.index(row, 0), role);
}

QVariant listValue(const FriendListModel& model, int uid, const QString& key)
{
    const int index = model.indexOfUid(uid);
    if (index < 0)
    {
        return {};
    }
    return model.get(index).value(key);
}

std::shared_ptr<FriendInfo> makeListDialog(int uid, const QString& name, const QString& dialogType)
{
    auto dialog =
        std::make_shared<FriendInfo>(uid, name, name, QStringLiteral("qrc:/res/head_1.png"), 0, QString(), QString());
    dialog->_dialog_type = dialogType;
    return dialog;
}

std::shared_ptr<TextChatMsg>
makeControllerIncomingBatch(int fromUid, int toUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
{
    auto batch = std::make_shared<TextChatMsg>(fromUid, toUid, QJsonArray{});
    batch->_chat_msgs = messages;
    return batch;
}
} // namespace

TEST(ChatFeatureControllerTest, BoundRequestsFanOutToHandlersExactlyOnce)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    HandlerCalls calls;

    configureCountingPorts(controller, calls);
    controller.bindRequests();
    controller.bindRequests();

    viewModel.ensureChatListInitialized();
    viewModel.ensureGroupsInitialized();
    viewModel.selectDialogByUid(1060);
    viewModel.selectChatIndex(2);
    viewModel.selectGroupIndex(3);
    viewModel.loadMoreChats();
    viewModel.loadMorePrivateHistory();
    viewModel.sendCurrentComposerPayload(QStringLiteral("hello"));
    viewModel.sendImageMessage();
    viewModel.sendFileMessage();
    viewModel.removePendingAttachment(QStringLiteral("att-1"));
    viewModel.clearPendingAttachments();
    viewModel.updateCurrentDraft(QStringLiteral("draft"));
    viewModel.toggleCurrentDialogPinned();
    viewModel.toggleCurrentDialogMuted();
    viewModel.toggleDialogPinnedByUid(-88);
    viewModel.toggleDialogMutedByUid(-89);
    viewModel.markDialogReadByUid(1061);
    viewModel.clearDialogDraftByUid(1062);
    viewModel.beginReplyMessage(QStringLiteral("m1"), QStringLiteral("Alice"), QStringLiteral("preview"));
    viewModel.cancelReplyMessage();
    viewModel.startVoiceChat();
    viewModel.startVideoChat();

    EXPECT_EQ(calls.ensureChatListInitialized, 0);
    EXPECT_EQ(calls.ensureGroupsInitialized, 1);
    EXPECT_EQ(calls.selectDialogByUid, 0);
    EXPECT_EQ(calls.selectChatIndex, 0);
    EXPECT_EQ(calls.selectGroupIndex, 0);
    EXPECT_EQ(calls.loadMoreChats, 0);
    EXPECT_EQ(calls.loadMorePrivateHistory, 1);
    EXPECT_EQ(calls.sendCurrentComposerPayload, 1);
    EXPECT_EQ(calls.sendImageMessage, 1);
    EXPECT_EQ(calls.sendFileMessage, 1);
    EXPECT_EQ(calls.removePendingAttachment, 1);
    EXPECT_EQ(calls.clearPendingAttachments, 1);
    EXPECT_EQ(calls.syncCurrentDraftText, 0);
    EXPECT_EQ(calls.syncCurrentDialogPinned, 0);
    EXPECT_EQ(calls.syncCurrentDialogMuted, 0);
    EXPECT_EQ(calls.startVoiceChat, 1);
    EXPECT_EQ(calls.startVideoChat, 1);
    EXPECT_EQ(calls.totalCalls(), 9);

    EXPECT_EQ(calls.selectedDialogUid, 0);
    EXPECT_EQ(calls.selectedChatIndex, 0);
    EXPECT_EQ(calls.selectedGroupIndex, 0);
    EXPECT_EQ(calls.sentComposerText, QStringLiteral("hello"));
    EXPECT_EQ(calls.removedAttachmentId, QStringLiteral("att-1"));
    EXPECT_TRUE(calls.syncedDraftText.isEmpty());
    EXPECT_FALSE(controller.hasPendingReply());
    EXPECT_FALSE(viewModel.hasPendingReply());
}

TEST(ChatFeatureControllerTest, SelectionEntrypointsDispatchThroughStoredSelectionPort)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    FriendListModel groupListModel;
    QMap<int, qint64> groupDialogUidMap;
    const int groupDialogUid = ConversationSyncService::resolveGroupDialogUid(groupDialogUidMap, 7001);

    auto friendInfo = makeListDialog(2002, QStringLiteral("Alice"), QStringLiteral("private"));
    friendInfo->_unread_count = 3;
    controller.chatListModel().upsertFriend(friendInfo);
    controller.dialogListModel().upsertFriend(friendInfo);

    auto groupDialog = makeListDialog(groupDialogUid, QStringLiteral("Team"), QStringLiteral("group"));
    groupDialog->_unread_count = 2;
    groupDialog->_mention_count = 1;
    groupListModel.upsertFriend(groupDialog);
    controller.dialogListModel().upsertFriend(groupDialog);

    auto groupInfo = std::make_shared<GroupInfoData>();
    groupInfo->_group_id = 7001;
    groupInfo->_name = QStringLiteral("Team");
    groupInfo->_group_code = QStringLiteral("G7001");

    int currentPrivateUid = 0;
    qint64 currentGroupId = 0;
    int loadPrivateMessages = 0;
    int openGroupCount = 0;
    int loadGroupHistory = 0;
    int groupReadAckCount = 0;
    int syncDraftCount = 0;

    ChatDialogSelectionPort port;
    port.groupDialogs.groupDialogByIndex = [&groupListModel](int index)
    {
        return groupListModel.get(index);
    };
    port.groupDialogs.indexOfGroupDialog = [&groupListModel](int dialogUid)
    {
        return groupListModel.indexOfUid(dialogUid);
    };
    port.groupDialogs.groupDialogItem = [&groupListModel](int dialogUid)
    {
        const int index = groupListModel.indexOfUid(dialogUid);
        return index >= 0 ? groupListModel.get(index) : QVariantMap();
    };
    port.groupDialogs.groupIdForDialogUid = [&groupDialogUidMap](int dialogUid)
    {
        return ConversationSyncService::groupIdForDialogUid(groupDialogUidMap, dialogUid);
    };
    port.groupDialogs.clearGroupUnread = [&groupListModel](int dialogUid)
    {
        groupListModel.clearUnread(dialogUid);
    };
    port.friendById = [friendInfo](int uid) -> std::shared_ptr<FriendInfo>
    {
        return uid == friendInfo->_uid ? friendInfo : nullptr;
    };
    port.groupById = [groupInfo](qint64 groupId) -> std::shared_ptr<GroupInfoData>
    {
        return groupId == groupInfo->_group_id ? groupInfo : nullptr;
    };
    port.setCurrentPrivatePeerUid = [&currentPrivateUid](int uid)
    {
        currentPrivateUid = uid;
    };
    port.setCurrentGroup = [&currentGroupId](qint64 groupId, const QString&, const QString&)
    {
        currentGroupId = groupId;
    };
    port.loadCurrentPrivateMessages = [&loadPrivateMessages]()
    {
        ++loadPrivateMessages;
    };
    port.openGroupConversation = [&openGroupCount](qint64)
    {
        ++openGroupCount;
        return static_cast<qint64>(9001);
    };
    port.sendGroupReadAck = [&groupReadAckCount](qint64, qint64)
    {
        ++groupReadAckCount;
    };
    port.loadGroupHistory = [&loadGroupHistory]()
    {
        ++loadGroupHistory;
    };
    port.syncCurrentDialogDraft = [&syncDraftCount]()
    {
        ++syncDraftCount;
    };
    controller.setDialogSelectionPort(std::move(port));

    controller.selectPrivateByIndex(0);

    EXPECT_EQ(currentPrivateUid, 2002);
    EXPECT_EQ(loadPrivateMessages, 1);
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("unreadCount")).toInt(), 0);

    controller.selectGroupByIndex(0);

    EXPECT_EQ(currentPrivateUid, 0);
    EXPECT_EQ(currentGroupId, 7001);
    EXPECT_EQ(openGroupCount, 1);
    EXPECT_EQ(groupReadAckCount, 1);
    EXPECT_EQ(loadGroupHistory, 1);
    EXPECT_EQ(listValue(controller.dialogListModel(), groupDialogUid, QStringLiteral("unreadCount")).toInt(), 0);
    EXPECT_EQ(listValue(controller.dialogListModel(), groupDialogUid, QStringLiteral("mentionCount")).toInt(), 0);
    EXPECT_GT(syncDraftCount, 0);
}

TEST(ChatFeatureControllerTest, BoundSelectionRequestsUseStoredSelectionPort)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    auto friendInfo = makeListDialog(2002, QStringLiteral("Alice"), QStringLiteral("private"));
    controller.chatListModel().upsertFriend(friendInfo);
    controller.dialogListModel().upsertFriend(friendInfo);

    int currentPrivateUid = 0;
    int loadPrivateMessages = 0;
    ChatDialogSelectionPort port;
    port.friendById = [friendInfo](int uid) -> std::shared_ptr<FriendInfo>
    {
        return uid == friendInfo->_uid ? friendInfo : nullptr;
    };
    port.setCurrentPrivatePeerUid = [&currentPrivateUid](int uid)
    {
        currentPrivateUid = uid;
    };
    port.loadCurrentPrivateMessages = [&loadPrivateMessages]()
    {
        ++loadPrivateMessages;
    };
    controller.setDialogSelectionPort(std::move(port));
    controller.bindRequests();

    viewModel.selectDialogByUid(2002);
    viewModel.selectChatIndex(0);

    EXPECT_EQ(currentPrivateUid, 2002);
    EXPECT_EQ(loadPrivateMessages, 2);
}

TEST(ChatFeatureControllerTest, CurrentMessageMutationFacadeUsesStoredPort)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    ChatMessageMutationSnapshot snapshot;
    snapshot.selfUid = 1001;
    snapshot.target.peerUid = 2002;

    int snapshotCalls = 0;
    int privateExistsCalls = 0;
    QString privateExistsMsgId;
    int privateStatusCalls = 0;
    int groupStatusCalls = 0;
    int dispatchCalls = 0;
    int dispatchedReqId = 0;
    QByteArray dispatchedPayload;

    ChatMessageMutationPort port;
    port.snapshot = [&snapshotCalls, &snapshot]()
    {
        ++snapshotCalls;
        return snapshot;
    };
    port.privateMessageExists = [&privateExistsCalls, &privateExistsMsgId](const QString& msgId)
    {
        ++privateExistsCalls;
        privateExistsMsgId = msgId;
        return true;
    };
    port.setPrivateStatus = [&privateStatusCalls](const QString&, bool)
    {
        ++privateStatusCalls;
    };
    port.setGroupStatus = [&groupStatusCalls](const QString&, bool)
    {
        ++groupStatusCalls;
    };
    port.dispatchPayload = [&dispatchCalls, &dispatchedReqId, &dispatchedPayload](int reqId, const QByteArray& payload)
    {
        ++dispatchCalls;
        dispatchedReqId = reqId;
        dispatchedPayload = payload;
    };
    controller.setMessageMutationPort(std::move(port));

    const MessageMutationCommandResult privateForward =
        controller.forwardCurrentMessage(QStringLiteral(" private-msg-1 "));

    EXPECT_TRUE(privateForward.success);
    EXPECT_TRUE(privateForward.dispatched);
    EXPECT_EQ(snapshotCalls, 1);
    EXPECT_EQ(privateExistsCalls, 1);
    EXPECT_EQ(privateExistsMsgId, QStringLiteral("private-msg-1"));
    EXPECT_EQ(dispatchCalls, 1);
    EXPECT_EQ(dispatchedReqId, 1083);
    QJsonObject payload = compactJsonObject(dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("msgid")).toString(), QStringLiteral("private-msg-1"));
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), 2002);
    EXPECT_FALSE(payload.value(QStringLiteral("client_msg_id")).toString().isEmpty());
    EXPECT_EQ(privateStatusCalls, 0);
    EXPECT_EQ(groupStatusCalls, 0);

    snapshot.target.peerUid = 0;
    snapshot.target.groupId = 7001;
    const MessageMutationCommandResult groupRevoke = controller.revokeCurrentMessage(QStringLiteral(" group-msg-1 "));

    EXPECT_TRUE(groupRevoke.success);
    EXPECT_TRUE(groupRevoke.dispatched);
    EXPECT_EQ(snapshotCalls, 2);
    EXPECT_EQ(privateExistsCalls, 1);
    EXPECT_EQ(dispatchCalls, 2);
    EXPECT_EQ(dispatchedReqId, 1067);
    payload = compactJsonObject(dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("msgid")).toString(), QStringLiteral("group-msg-1"));
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
    EXPECT_FALSE(payload.contains(QStringLiteral("peer_uid")));
    EXPECT_EQ(privateStatusCalls, 0);
    EXPECT_EQ(groupStatusCalls, 0);
}

TEST(ChatFeatureControllerTest, DialogListResponseOwnsFeatureModelAndDecorationCallbacks)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    auto existingDialog = makeListDialog(3003, QStringLiteral("Existing"), QStringLiteral("private"));
    existingDialog->_last_msg = QStringLiteral("existing preview");
    controller.dialogListModel().upsertFriend(existingDialog);
    controller.setMentionCountForDialog(3003, 4);

    QJsonArray dialogs;
    QJsonObject privateDialog;
    privateDialog.insert(QStringLiteral("dialog_type"), QStringLiteral("private"));
    privateDialog.insert(QStringLiteral("peer_uid"), 2002);
    privateDialog.insert(QStringLiteral("title"), QStringLiteral("Alice"));
    privateDialog.insert(QStringLiteral("avatar"), QStringLiteral("alice.png"));
    privateDialog.insert(QStringLiteral("last_msg_preview"), QStringLiteral("hello"));
    privateDialog.insert(QStringLiteral("unread_count"), 2);
    privateDialog.insert(QStringLiteral("pinned_rank"), 3);
    privateDialog.insert(QStringLiteral("draft_text"), QStringLiteral("draft from server"));
    privateDialog.insert(QStringLiteral("last_msg_ts"), static_cast<qint64>(1700000000));
    privateDialog.insert(QStringLiteral("mute_state"), 1);
    dialogs.push_back(privateDialog);

    QJsonObject payload;
    payload.insert(QStringLiteral("error"), 0);
    payload.insert(QStringLiteral("dialogs"), dialogs);

    ChatDialogListResponseContext context;
    context.bootstrappingDialog = true;
    context.hasCurrentDialog = false;

    int dialogsReady = 0;
    int chatListInitialized = 0;
    int selectDialogUid = 0;
    int syncDraft = 0;
    int flushIncoming = 0;
    ChatDialogListAppPort appPort;
    appPort.setDialogsReady = [&dialogsReady](bool ready)
    {
        dialogsReady += ready ? 1 : 0;
    };
    appPort.setChatListInitialized = [&chatListInitialized](bool initialized)
    {
        chatListInitialized += initialized ? 1 : 0;
    };
    appPort.selectDialogByUid = [&selectDialogUid](int dialogUid)
    {
        selectDialogUid = dialogUid;
    };
    appPort.syncCurrentDialogDraft = [&syncDraft]()
    {
        ++syncDraft;
    };
    appPort.flushIncomingMessages = [&flushIncoming]()
    {
        ++flushIncoming;
    };

    const ChatDialogListResponseEffect effect = controller.handleDialogListResponse(payload, context, appPort);

    ASSERT_TRUE(effect.dialogsReady);
    EXPECT_EQ(dialogsReady, 1);
    EXPECT_EQ(chatListInitialized, 1);
    EXPECT_EQ(selectDialogUid, 2002);
    EXPECT_EQ(syncDraft, 1);
    EXPECT_EQ(flushIncoming, 1);
    EXPECT_EQ(controller.chatListModel().indexOfUid(2002), 0);
    EXPECT_GE(controller.dialogListModel().indexOfUid(2002), 0);
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("lastMsg")).toString(),
                        QStringLiteral("hello"));
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("unreadCount")).toInt(), 0);
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("draftText")).toString(),
                        QStringLiteral("draft from server"));
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("muteState")).toInt(), 1);
    EXPECT_EQ(listValue(controller.dialogListModel(), 3003, QStringLiteral("mentionCount")).toInt(), 4);
}

TEST(ChatFeatureControllerTest, GroupDialogIdentityMapIsFeatureOwned)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    const int firstDialogUid = controller.resolveGroupDialogUid(7001);
    const int secondDialogUid = controller.resolveGroupDialogUid(7001);

    ASSERT_LT(firstDialogUid, 0);
    EXPECT_EQ(firstDialogUid, secondDialogUid);
    EXPECT_EQ(controller.groupIdForDialogUid(firstDialogUid), 7001);
    ASSERT_NE(controller.groupDialogUidMapPtr(), nullptr);
    EXPECT_EQ(controller.groupDialogUidMapPtr(), &controller.groupDialogUidMap());
    EXPECT_TRUE(controller.groupDialogUidMap().contains(firstDialogUid));

    controller.removeGroupDialogUid(firstDialogUid);
    EXPECT_FALSE(controller.groupDialogUidMap().contains(firstDialogUid));
    EXPECT_EQ(controller.groupIdForDialogUid(firstDialogUid), 7001);

    controller.resetGroupDialogIdentityMap();
    EXPECT_TRUE(controller.groupDialogUidMap().isEmpty());
}

TEST(ChatFeatureControllerTest, GroupDialogIdentityFeedsSnapshotRefreshAndDialogListResponse)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    auto group = std::make_shared<GroupInfoData>();
    group->_group_id = 7001;
    group->_name = QStringLiteral("Team");
    group->_icon = QStringLiteral("team.png");
    group->_last_msg = QStringLiteral("snapshot preview");
    const ChatDialogSnapshotRefreshResult refreshResult = controller.replaceDialogListFromSnapshots({}, {group});
    const int dialogUid = controller.resolveGroupDialogUid(7001);

    EXPECT_EQ(refreshResult.groupCount, 1);
    EXPECT_LT(dialogUid, 0);
    EXPECT_GE(controller.dialogListModel().indexOfUid(dialogUid), 0);

    QJsonArray dialogs;
    QJsonObject groupDialog;
    groupDialog.insert(QStringLiteral("dialog_type"), QStringLiteral("group"));
    groupDialog.insert(QStringLiteral("group_id"), static_cast<qint64>(7001));
    groupDialog.insert(QStringLiteral("title"), QStringLiteral("Team"));
    groupDialog.insert(QStringLiteral("avatar"), QStringLiteral("team.png"));
    groupDialog.insert(QStringLiteral("last_msg_preview"), QStringLiteral("server preview"));
    groupDialog.insert(QStringLiteral("unread_count"), 3);
    dialogs.push_back(groupDialog);

    QJsonObject payload;
    payload.insert(QStringLiteral("error"), 0);
    payload.insert(QStringLiteral("dialogs"), dialogs);

    ChatDialogListResponseContext context;
    context.groupSnapshot = {group};
    ChatDialogListAppPort appPort;
    const ChatDialogListResponseEffect effect = controller.handleDialogListResponse(payload, context, appPort);

    ASSERT_TRUE(effect.dialogsReady);
    EXPECT_EQ(controller.groupIdForDialogUid(dialogUid), 7001);
    EXPECT_GE(controller.dialogListModel().indexOfUid(dialogUid), 0);
    EXPECT_EQ(listValue(controller.dialogListModel(), dialogUid, QStringLiteral("lastMsg")).toString(),
                        QStringLiteral("server preview"));
}

TEST(ChatFeatureControllerTest, ChatListBootstrapOwnsInitialPageAndFlushOrder)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    bool initialized = false;
    bool canLoadMore = false;
    int nextPageCalls = 0;
    int markLoadedCalls = 0;
    int initializedCalls = 0;
    int canLoadMoreCalls = 0;
    int flushCalls = 0;
    std::vector<QString> order;

    ChatListPagingPort port;
    port.snapshot = [&initialized]
    {
        ChatListPagingSnapshot snapshot;
        snapshot.initialized = initialized;
        return snapshot;
    };
    port.nextPage = [&nextPageCalls]
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{
            makeListDialog(2002, QStringLiteral("Alice"), QStringLiteral("private"))};
    };
    port.loadFinished = []
    {
        return false;
    };
    port.markPageLoaded = [&markLoadedCalls, &order]
    {
        ++markLoadedCalls;
        order.push_back(QStringLiteral("mark"));
    };
    port.setInitialized = [&initialized, &initializedCalls, &order](bool value)
    {
        initialized = value;
        ++initializedCalls;
        order.push_back(QStringLiteral("initialized"));
    };
    port.setCanLoadMore = [&canLoadMore, &canLoadMoreCalls, &order](bool value)
    {
        canLoadMore = value;
        ++canLoadMoreCalls;
        order.push_back(QStringLiteral("canLoadMore"));
    };
    port.flushIncomingMessages = [&flushCalls, &order]
    {
        ++flushCalls;
        order.push_back(QStringLiteral("flush"));
    };
    controller.setChatListPagingPort(std::move(port));

    const ChatListPagingResult result = controller.ensureChatListInitialized();

    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.skipped);
    EXPECT_TRUE(result.initialized);
    EXPECT_TRUE(result.canLoadMoreProjected);
    EXPECT_TRUE(result.incomingFlushed);
    EXPECT_EQ(result.itemCount, 1);
    EXPECT_EQ(nextPageCalls, 1);
    EXPECT_EQ(markLoadedCalls, 1);
    EXPECT_EQ(initializedCalls, 1);
    EXPECT_EQ(canLoadMoreCalls, 1);
    EXPECT_EQ(flushCalls, 1);
    EXPECT_TRUE(initialized);
    EXPECT_TRUE(canLoadMore);
    EXPECT_EQ(controller.chatListModel().indexOfUid(2002), 0);
    ASSERT_EQ(order.size(), 4);
    EXPECT_EQ(order.at(0), QStringLiteral("mark"));
    EXPECT_EQ(order.at(1), QStringLiteral("initialized"));
    EXPECT_EQ(order.at(2), QStringLiteral("canLoadMore"));
    EXPECT_EQ(order.at(3), QStringLiteral("flush"));
}

TEST(ChatFeatureControllerTest, ChatListLoadMoreAppendsPageAndProjectsLoading)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    controller.chatListModel().setFriends({makeListDialog(2002, QStringLiteral("Alice"), QStringLiteral("private"))});

    bool loading = false;
    bool loadFinished = false;
    int nextPageCalls = 0;
    int markLoadedCalls = 0;
    int loadingCalls = 0;
    int canLoadMoreCalls = 0;
    std::vector<bool> loadingValues;

    ChatListPagingPort port;
    port.snapshot = [&loading]
    {
        ChatListPagingSnapshot snapshot;
        snapshot.initialized = true;
        snapshot.loading = loading;
        return snapshot;
    };
    port.nextPage = [&nextPageCalls]
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{
            makeListDialog(2003, QStringLiteral("Bob"), QStringLiteral("private"))};
    };
    port.loadFinished = [&loadFinished]
    {
        return loadFinished;
    };
    port.markPageLoaded = [&markLoadedCalls]
    {
        ++markLoadedCalls;
    };
    port.setLoading = [&loading, &loadingCalls, &loadingValues](bool value)
    {
        loading = value;
        ++loadingCalls;
        loadingValues.push_back(value);
    };
    port.setCanLoadMore = [&canLoadMoreCalls](bool)
    {
        ++canLoadMoreCalls;
    };
    controller.setChatListPagingPort(std::move(port));

    const ChatListPagingResult result = controller.loadMoreChats();

    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.skipped);
    EXPECT_TRUE(result.pageAppended);
    EXPECT_TRUE(result.loadingProjected);
    EXPECT_TRUE(result.canLoadMoreProjected);
    EXPECT_EQ(result.itemCount, 1);
    EXPECT_EQ(nextPageCalls, 1);
    EXPECT_EQ(markLoadedCalls, 1);
    EXPECT_EQ(loadingCalls, 2);
    ASSERT_EQ(loadingValues.size(), 2);
    EXPECT_TRUE(loadingValues.at(0));
    EXPECT_FALSE(loadingValues.at(1));
    EXPECT_EQ(canLoadMoreCalls, 1);
    EXPECT_EQ(controller.chatListModel().indexOfUid(2002), 0);
    EXPECT_EQ(controller.chatListModel().indexOfUid(2003), 1);
}

TEST(ChatFeatureControllerTest, ChatListLoadMoreSkipsWhenLoadingOrFinished)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    bool loading = true;
    bool loadFinished = false;
    int nextPageCalls = 0;
    int loadingCalls = 0;
    int canLoadMoreCalls = 0;

    ChatListPagingPort port;
    port.snapshot = [&loading]
    {
        ChatListPagingSnapshot snapshot;
        snapshot.initialized = true;
        snapshot.loading = loading;
        return snapshot;
    };
    port.nextPage = [&nextPageCalls]
    {
        ++nextPageCalls;
        return std::vector<std::shared_ptr<FriendInfo>>{};
    };
    port.loadFinished = [&loadFinished]
    {
        return loadFinished;
    };
    port.setLoading = [&loadingCalls](bool)
    {
        ++loadingCalls;
    };
    port.setCanLoadMore = [&canLoadMoreCalls](bool)
    {
        ++canLoadMoreCalls;
    };
    controller.setChatListPagingPort(std::move(port));

    ChatListPagingResult result = controller.loadMoreChats();
    EXPECT_TRUE(result.skipped);
    EXPECT_FALSE(result.pageAppended);
    EXPECT_EQ(nextPageCalls, 0);
    EXPECT_EQ(loadingCalls, 0);
    EXPECT_EQ(canLoadMoreCalls, 0);

    loading = false;
    loadFinished = true;
    result = controller.loadMoreChats();
    EXPECT_TRUE(result.skipped);
    EXPECT_FALSE(result.pageAppended);
    EXPECT_EQ(nextPageCalls, 0);
    EXPECT_EQ(loadingCalls, 0);
    EXPECT_EQ(canLoadMoreCalls, 1);
}

TEST(ChatFeatureControllerTest, ReplyRequestsAreHandledInsideFeatureController)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    controller.bindRequests();
    viewModel.beginReplyMessage(QStringLiteral(" msg-1 "), QStringLiteral(" Alice "), QStringLiteral("preview"));

    EXPECT_TRUE(controller.hasPendingReply());
    EXPECT_EQ(controller.replyToMsgId(), QStringLiteral("msg-1"));
    EXPECT_EQ(controller.replyTargetName(), QStringLiteral("Alice"));
    EXPECT_EQ(controller.replyPreviewText(), QStringLiteral("preview"));
    EXPECT_TRUE(viewModel.hasPendingReply());
    EXPECT_EQ(viewModel.replyTargetName(), QStringLiteral("Alice"));
    EXPECT_EQ(viewModel.replyPreviewText(), QStringLiteral("preview"));

    viewModel.cancelReplyMessage();

    EXPECT_FALSE(controller.hasPendingReply());
    EXPECT_FALSE(viewModel.hasPendingReply());
    EXPECT_TRUE(viewModel.replyTargetName().isEmpty());
    EXPECT_TRUE(viewModel.replyPreviewText().isEmpty());
}

TEST(ChatFeatureControllerTest, SendPrivateTextUsesFeatureOwnedHistoryCursorByDefault)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    ChatMessageModel messageModel;
    auto friendInfo = makeListDialog(2002, QStringLiteral("Alice"), QStringLiteral("private"));
    friendInfo->_unread_count = 3;
    controller.chatListModel().upsertFriend(friendInfo);
    controller.dialogListModel().upsertFriend(friendInfo);

    OutgoingChatPacket dispatchedPacket;
    controller.setPrivatePacketDispatcher(
        [&dispatchedPacket](const OutgoingChatPacket& packet, QString*)
        {
            dispatchedPacket = packet;
            return true;
        });

    int appendedPeerUid = 0;
    std::vector<std::shared_ptr<TextChatData>> appendedMessages;
    controller.setPrivateAppendFriendMessages(
        [&appendedPeerUid, &appendedMessages](int peerUid, const std::vector<std::shared_ptr<TextChatData>>& messages)
        {
            appendedPeerUid = peerUid;
            appendedMessages = messages;
        });

    PrivateChatSendRequest request;
    request.peerUid = 2002;
    request.content = QStringLiteral("hello");

    PrivateChatSendDependencies dependencies;
    dependencies.selfUid = 1001;
    dependencies.messageModel = &messageModel;

    const PrivateChatSendResult result = controller.sendPrivateText(request, dependencies);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(dispatchedPacket.msgId, result.packet.msgId);
    EXPECT_EQ(appendedPeerUid, 2002);
    ASSERT_EQ(appendedMessages.size(), 1U);
    EXPECT_EQ(messageModel.rowCount(), 1);
    EXPECT_EQ(controller.privateHistoryState().beforeTs, result.packet.createdAt);
    EXPECT_EQ(controller.privateHistoryState().beforeMsgId, result.packet.msgId);
    EXPECT_EQ(listValue(controller.chatListModel(), 2002, QStringLiteral("lastMsg")).toString(),
                        QStringLiteral("hello"));
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("lastMsg")).toString(),
                        QStringLiteral("hello"));
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("unreadCount")).toInt(), 0);
}

TEST(ChatFeatureControllerTest, PrivateIncomingEnsuresMissingFriendInsideFeatureController)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    ChatMessageModel messageModel;
    QHash<int, std::shared_ptr<FriendInfo>> friends;
    int addFriendCount = 0;
    int upsertContactCount = 0;
    int appendedPeerUid = 0;
    std::vector<std::shared_ptr<TextChatData>> appendedMessages;

    PrivateChatEventDependencies dependencies;
    dependencies.messageModel = &messageModel;
    dependencies.friendById = [&friends](int uid)
    {
        return friends.value(uid);
    };
    dependencies.addFriend = [&friends, &addFriendCount](std::shared_ptr<AuthInfo> authInfo)
    {
        ++addFriendCount;
        if (authInfo)
        {
            friends.insert(authInfo->_uid, std::make_shared<FriendInfo>(authInfo));
        }
    };
    dependencies.upsertContact = [&upsertContactCount](std::shared_ptr<AuthInfo>)
    {
        ++upsertContactCount;
    };
    dependencies.appendFriendMessages =
        [&friends, &appendedPeerUid, &appendedMessages](int peerUid,
                                                        const std::vector<std::shared_ptr<TextChatData>>& messages)
    {
        appendedPeerUid = peerUid;
        appendedMessages = messages;
        const auto friendInfo = friends.value(peerUid);
        if (friendInfo)
        {
            friendInfo->AppendChatMsgs(messages);
        }
    };

    const auto incoming = std::make_shared<TextChatData>(
        QStringLiteral("incoming-missing"), QStringLiteral("from unknown"), 3003, 1001, QString(), 9001);
    PrivateIncomingMessageRequest request;
    request.context.selfUid = 1001;
    request.context.currentPeerUid = 2002;
    request.context.currentGroupId = 0;
    request.message = makeControllerIncomingBatch(3003, 1001, {incoming});

    const PrivateIncomingMessageResult result =
        controller.handlePrivateIncomingMessage(request, std::move(dependencies));

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.ensuredFriend);
    EXPECT_EQ(addFriendCount, 1);
    EXPECT_EQ(upsertContactCount, 1);
    EXPECT_TRUE(friends.contains(3003));
    EXPECT_EQ(appendedPeerUid, 3003);
    ASSERT_EQ(appendedMessages.size(), 1U);
    EXPECT_EQ(controller.chatListModel().indexOfUid(3003), 0);
    EXPECT_EQ(controller.dialogListModel().indexOfUid(3003), 0);
    EXPECT_EQ(listValue(controller.dialogListModel(), 3003, QStringLiteral("lastMsg")).toString(),
                        QStringLiteral("from unknown"));
    EXPECT_EQ(listValue(controller.dialogListModel(), 3003, QStringLiteral("unreadCount")).toInt(), 1);
}

TEST(ChatFeatureControllerTest, DialogRuntimeStateIsFeatureOwnedAndClearable)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    const QVariantList attachments{QStringLiteral("attachment")};
    EXPECT_TRUE(controller.setCurrentDraftText(QStringLiteral("draft")));
    EXPECT_TRUE(controller.setCurrentPendingAttachments(attachments));
    controller.setPendingAttachmentsForDialog(42, attachments);
    EXPECT_TRUE(controller.setCurrentDialogPinned(true));
    EXPECT_TRUE(controller.setCurrentDialogMuted(true));
    EXPECT_TRUE(
        controller.setPendingReplyContext(QStringLiteral("msg-1"), QStringLiteral("Alice"), QStringLiteral("preview")));
    controller.updateDraftForDialog(42, QStringLiteral("draft"));
    controller.applyRemotePinnedMeta(42, 100);
    controller.applyRemoteDraftMeta(42, QStringLiteral("draft"), 1);
    controller.setMentionCountForDialog(42, 2);
    EXPECT_TRUE(controller.updateLastEmittedDialogUid(42));

    controller.clearDialogRuntimeState();

    EXPECT_TRUE(controller.currentDraftText().isEmpty());
    EXPECT_TRUE(controller.currentPendingAttachments().isEmpty());
    EXPECT_FALSE(controller.currentDialogPinned());
    EXPECT_FALSE(controller.currentDialogMuted());
    EXPECT_FALSE(controller.hasPendingReply());
    EXPECT_TRUE(controller.replyTargetName().isEmpty());
    EXPECT_TRUE(controller.replyPreviewText().isEmpty());

    const ChatDialogRuntimeSnapshot snapshot = controller.snapshotForDialog(42);
    EXPECT_TRUE(snapshot.hasDialog);
    EXPECT_TRUE(snapshot.draftText.isEmpty());
    EXPECT_TRUE(snapshot.pendingAttachments.isEmpty());
    EXPECT_FALSE(snapshot.pinned);
    EXPECT_FALSE(snapshot.muted);

    const DialogDecorationState decorationState = controller.dialogDecorationState();
    ASSERT_NE(decorationState.draftMap, nullptr);
    ASSERT_NE(decorationState.mentionMap, nullptr);
    ASSERT_NE(decorationState.serverPinnedMap, nullptr);
    ASSERT_NE(decorationState.serverMuteMap, nullptr);
    ASSERT_NE(decorationState.localPinnedSet, nullptr);
    EXPECT_TRUE(decorationState.draftMap->isEmpty());
    EXPECT_TRUE(decorationState.mentionMap->isEmpty());
    EXPECT_TRUE(decorationState.serverPinnedMap->isEmpty());
    EXPECT_TRUE(decorationState.serverMuteMap->isEmpty());
    EXPECT_TRUE(decorationState.localPinnedSet->isEmpty());
    EXPECT_TRUE(controller.updateLastEmittedDialogUid(42));
}

TEST(ChatFeatureControllerTest, ClearPrivateConversationIfSelectedSkipsOtherFriends)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    int currentPeerUid = 2002;
    int messageClears = 0;
    int peerNameResets = 0;
    int emitted = 0;

    ChatPrivateConversationClearPort port;
    port.currentPrivatePeerUid = [&currentPeerUid]()
    {
        return currentPeerUid;
    };
    port.setCurrentPrivatePeerUid = [&currentPeerUid](int uid)
    {
        currentPeerUid = uid;
    };
    port.clearMessageModel = [&messageClears]()
    {
        ++messageClears;
    };
    port.setCurrentPeerName = [&peerNameResets](const QString&)
    {
        ++peerNameResets;
    };
    port.emitCurrentDialogUidChangedIfNeeded = [&emitted]()
    {
        ++emitted;
    };

    ChatPrivateConversationClearRequest request;
    request.friendUid = 3003;
    const ChatPrivateConversationClearResult result = controller.clearPrivateConversationIfSelected(request, port);

    EXPECT_FALSE(result.success);
    EXPECT_TRUE(result.skipped);
    EXPECT_EQ(currentPeerUid, 2002);
    EXPECT_EQ(messageClears, 0);
    EXPECT_EQ(peerNameResets, 0);
    EXPECT_EQ(emitted, 0);
}

TEST(ChatFeatureControllerTest, ClearPrivateConversationIfSelectedOwnsResetOrder)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    int currentPeerUid = 2002;
    QStringList calls;
    QString peerName = QStringLiteral("Alice");
    QString peerIcon = QStringLiteral("alice.png");
    QString currentDraft = QStringLiteral("draft");
    bool currentPinned = true;
    bool currentMuted = true;

    ChatPrivateConversationClearPort port;
    port.currentPrivatePeerUid = [&currentPeerUid]()
    {
        return currentPeerUid;
    };
    port.setCurrentPrivatePeerUid = [&currentPeerUid, &calls](int uid)
    {
        currentPeerUid = uid;
        calls.push_back(QStringLiteral("clear-current-peer"));
    };
    port.clearMessageModel = [&calls]()
    {
        calls.push_back(QStringLiteral("clear-message-model"));
    };
    port.setCurrentPeerName = [&peerName, &calls](const QString& name)
    {
        peerName = name;
        calls.push_back(QStringLiteral("reset-peer-name"));
    };
    port.setCurrentPeerIcon = [&peerIcon, &calls](const QString& icon)
    {
        peerIcon = icon;
        calls.push_back(QStringLiteral("reset-peer-icon"));
    };
    port.setCurrentDraftText = [&currentDraft, &calls](const QString& text)
    {
        currentDraft = text;
        calls.push_back(QStringLiteral("reset-draft"));
    };
    port.setCurrentDialogPinned = [&currentPinned, &calls](bool pinned)
    {
        currentPinned = pinned;
        calls.push_back(QStringLiteral("reset-pinned"));
    };
    port.setCurrentDialogMuted = [&currentMuted, &calls](bool muted)
    {
        currentMuted = muted;
        calls.push_back(QStringLiteral("reset-muted"));
    };
    port.emitCurrentDialogUidChangedIfNeeded = [&calls]()
    {
        calls.push_back(QStringLiteral("emit-current-dialog-uid"));
    };

    ChatPrivateConversationClearRequest request;
    request.friendUid = 2002;
    const ChatPrivateConversationClearResult result = controller.clearPrivateConversationIfSelected(request, port);

    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.skipped);
    EXPECT_TRUE(result.currentPrivateConversationCleared);
    EXPECT_TRUE(result.messageModelCleared);
    EXPECT_TRUE(result.peerProjectionReset);
    EXPECT_TRUE(result.runtimeProjectionReset);
    EXPECT_TRUE(result.dialogUidEmitted);
    EXPECT_EQ(currentPeerUid, 0);
    EXPECT_TRUE(peerName.isEmpty());
    EXPECT_EQ(peerIcon, QStringLiteral("qrc:/res/head_1.png"));
    EXPECT_TRUE(currentDraft.isEmpty());
    EXPECT_FALSE(currentPinned);
    EXPECT_FALSE(currentMuted);
    EXPECT_EQ(calls,
              QStringList({QStringLiteral("clear-current-peer"),
                  QStringLiteral("clear-message-model"),
                      QStringLiteral("reset-peer-name"),
                          QStringLiteral("reset-peer-icon"),
                              QStringLiteral("reset-draft"),
                                             QStringLiteral("reset-pinned"),
                                                            QStringLiteral("reset-muted"),
                                                                           QStringLiteral("emit-current-dialog-uid")}));
}

TEST(ChatFeatureControllerTest, DialogMetaResponsesOwnDraftMuteAndPinEffects)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    auto dialogInfo = std::make_shared<FriendInfo>(
        2002,
        QStringLiteral("Alice"), QStringLiteral("Alice"), QStringLiteral("alice.png"), 0, QString(), QString());
    dialogInfo->_dialog_type = QStringLiteral("private");
    controller.dialogListModel().upsertFriend(dialogInfo);

    QString currentDraft;
    bool currentMuted = false;
    bool currentPinned = false;
    int savedOwnerUid = 0;
    int refreshCount = 0;
    ChatDialogMetaResponseDependencies dependencies;
    dependencies.syncCurrentDraftText = [&currentDraft](const QString& draft)
    {
        currentDraft = draft;
    };
    dependencies.syncCurrentDialogMuted = [&currentMuted](bool muted)
    {
        currentMuted = muted;
    };
    dependencies.syncCurrentDialogPinned = [&currentPinned](bool pinned)
    {
        currentPinned = pinned;
    };
    dependencies.savePersistentDialogStore = [&savedOwnerUid](int ownerUid)
    {
        savedOwnerUid = ownerUid;
    };
    dependencies.refreshDialogModel = [&refreshCount]()
    {
        ++refreshCount;
    };

    ChatDialogMetaResponseRequest draftRequest;
    draftRequest.reqId = 1073;
    draftRequest.currentDialogUid = 2002;
    draftRequest.ownerUid = 1001;
    draftRequest.payload.insert(QStringLiteral("dialog_type"), QStringLiteral("private"));
    draftRequest.payload.insert(QStringLiteral("peer_uid"), 2002);
    draftRequest.payload.insert(QStringLiteral("draft_text"), QStringLiteral("remote draft"));
    draftRequest.payload.insert(QStringLiteral("mute_state"), 1);

    const ChatDialogMetaResponseResult draftResult = controller.handleDialogMetaResponse(draftRequest, dependencies);

    ASSERT_TRUE(draftResult.success);
    EXPECT_TRUE(draftResult.draftPath);
    EXPECT_TRUE(draftResult.runtimeUpdated);
    EXPECT_TRUE(draftResult.dialogModelUpdated);
    EXPECT_TRUE(draftResult.currentDialog);
    EXPECT_TRUE(draftResult.currentDraftUpdated);
    EXPECT_TRUE(draftResult.currentMutedUpdated);
    EXPECT_TRUE(draftResult.persistentStoreSaved);
    EXPECT_EQ(currentDraft, QStringLiteral("remote draft"));
    EXPECT_TRUE(currentMuted);
    EXPECT_EQ(savedOwnerUid, 1001);
    EXPECT_EQ(controller.snapshotForDialog(2002).draftText, QStringLiteral("remote draft"));
    EXPECT_TRUE(controller.snapshotForDialog(2002).muted);
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("draftText")).toString(),
                        QStringLiteral("remote draft"));
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("muteState")).toInt(), 1);

    ChatDialogMetaResponseRequest pinRequest;
    pinRequest.reqId = 1075;
    pinRequest.currentDialogUid = 2002;
    pinRequest.ownerUid = 1001;
    pinRequest.payload.insert(QStringLiteral("dialog_type"), QStringLiteral("private"));
    pinRequest.payload.insert(QStringLiteral("peer_uid"), 2002);
    pinRequest.payload.insert(QStringLiteral("pinned_rank"), 88);

    const ChatDialogMetaResponseResult pinResult = controller.handleDialogMetaResponse(pinRequest, dependencies);

    ASSERT_TRUE(pinResult.success);
    EXPECT_TRUE(pinResult.pinPath);
    EXPECT_TRUE(pinResult.runtimeUpdated);
    EXPECT_TRUE(pinResult.currentPinnedUpdated);
    EXPECT_TRUE(pinResult.persistentStoreSaved);
    EXPECT_TRUE(pinResult.dialogModelRefreshed);
    EXPECT_TRUE(currentPinned);
    EXPECT_EQ(refreshCount, 1);
    EXPECT_TRUE(controller.snapshotForDialog(2002).pinned);
}

TEST(ChatFeatureControllerTest, DialogRuntimeCommandsNormalizeDraftAndSnapshotState)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    const ChatDialogDraftResult invalidDraft = controller.updateDraftForDialog(0, QStringLiteral("ignored"));
    EXPECT_FALSE(invalidDraft.valid);

    const QString longDraft(2005, QLatin1Char('x'));
    const ChatDialogDraftResult draftResult = controller.updateDraftForDialog(42, longDraft);
    ASSERT_TRUE(draftResult.valid);
    EXPECT_EQ(draftResult.draftText.size(), 2000);
    EXPECT_EQ(controller.draftForDialog(42).size(), 2000);

    const QVariantList attachments{QStringLiteral("a"), QStringLiteral("b")};
    controller.setPendingAttachmentsForDialog(42, attachments);
    ChatDialogRuntimeSnapshot snapshot = controller.snapshotForDialog(42);
    ASSERT_TRUE(snapshot.hasDialog);
    EXPECT_EQ(snapshot.pendingAttachments, attachments);

    const ChatDialogPinnedResult pinned = controller.togglePinnedForDialog(42);
    ASSERT_TRUE(pinned.valid);
    EXPECT_TRUE(pinned.pinned);
    EXPECT_GT(pinned.pinnedRank, 0);
    snapshot = controller.snapshotForDialog(42);
    EXPECT_TRUE(snapshot.pinned);

    const ChatDialogMutedResult muted = controller.toggleMutedForDialog(42);
    ASSERT_TRUE(muted.valid);
    EXPECT_TRUE(muted.muted);
    EXPECT_EQ(muted.muteState, 1);
    snapshot = controller.snapshotForDialog(42);
    EXPECT_TRUE(snapshot.muted);

    const ChatDialogDraftResult cleared = controller.updateDraftForDialog(42, QStringLiteral("   "));
    ASSERT_TRUE(cleared.valid);
    EXPECT_TRUE(cleared.draftText.isEmpty());
    EXPECT_TRUE(controller.draftForDialog(42).isEmpty());

    controller.clearPendingAttachmentsForDialog(42);
    snapshot = controller.snapshotForDialog(42);
    EXPECT_TRUE(snapshot.pendingAttachments.isEmpty());
}

TEST(ChatFeatureControllerTest, DialogRuntimePayloadBuildersUseChatOwnedSchemas)
{
    const QByteArray privateDraft =
        ChatFeatureController::buildDraftSyncPayload(1001, QStringLiteral("private"), 2002, 0, QStringLiteral("draft"));
    QJsonObject obj = QJsonDocument::fromJson(privateDraft).object();
    EXPECT_EQ(obj.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(obj.value(QStringLiteral("dialog_type")).toString(), QStringLiteral("private"));
    EXPECT_EQ(obj.value(QStringLiteral("peer_uid")).toInt(), 2002);
    EXPECT_EQ(obj.value(QStringLiteral("draft_text")).toString(), QStringLiteral("draft"));
    EXPECT_FALSE(obj.contains(QStringLiteral("mute_state")));

    const QByteArray groupDraft =
        ChatFeatureController::buildDraftSyncPayload(1001, QStringLiteral("group"), 0, 7001, QStringLiteral(""), 1);
    obj = QJsonDocument::fromJson(groupDraft).object();
    EXPECT_EQ(obj.value(QStringLiteral("dialog_type")).toString(), QStringLiteral("group"));
    EXPECT_EQ(obj.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
    EXPECT_EQ(obj.value(QStringLiteral("draft_text")).toString(), QString());
    EXPECT_EQ(obj.value(QStringLiteral("mute_state")).toInt(), 1);

    const QByteArray pinPayload =
        ChatFeatureController::buildPinDialogPayload(1001, QStringLiteral("private"), 2002, 0, 123);
    obj = QJsonDocument::fromJson(pinPayload).object();
    EXPECT_EQ(obj.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(obj.value(QStringLiteral("dialog_type")).toString(), QStringLiteral("private"));
    EXPECT_EQ(obj.value(QStringLiteral("peer_uid")).toInt(), 2002);
    EXPECT_EQ(obj.value(QStringLiteral("pinned_rank")).toInt(), 123);
}

TEST(ChatFeatureControllerTest, DialogRuntimePayloadBuildersRejectInvalidTargets)
{
    EXPECT_TRUE(
        ChatFeatureController::buildDraftSyncPayload(0, QStringLiteral("private"), 2002, 0, QString()).isEmpty());
    EXPECT_TRUE(
        ChatFeatureController::buildDraftSyncPayload(1001, QStringLiteral("private"), 0, 0, QString()).isEmpty());
    EXPECT_TRUE(ChatFeatureController::buildDraftSyncPayload(1001, QStringLiteral("group"), 0, 0, QString()).isEmpty());
    EXPECT_TRUE(
        ChatFeatureController::buildDraftSyncPayload(1001, QStringLiteral("unknown"), 2002, 7001, QString()).isEmpty());
    EXPECT_TRUE(ChatFeatureController::buildPinDialogPayload(0, QStringLiteral("private"), 2002, 0, 1).isEmpty());
    EXPECT_TRUE(ChatFeatureController::buildPinDialogPayload(1001, QStringLiteral("private"), 0, 0, 1).isEmpty());
    EXPECT_TRUE(ChatFeatureController::buildPinDialogPayload(1001, QStringLiteral("group"), 0, 0, 1).isEmpty());
}

TEST(ChatFeatureControllerTest, DialogRuntimeCommandsOwnModelDispatchAndCurrentStateEffects)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    auto dialogInfo = std::make_shared<FriendInfo>(
        2002,
        QStringLiteral("Alice"), QStringLiteral("Alice"), QStringLiteral("alice.png"), 0, QString(), QString());
    dialogInfo->_dialog_type = QStringLiteral("private");
    controller.dialogListModel().upsertFriend(dialogInfo);

    QByteArray draftPayload;
    QByteArray pinPayload;
    int savedOwnerUid = 0;
    int refreshCount = 0;
    HandlerCalls calls;
    ChatDialogRuntimePort port;
    port.syncCurrentDraftText = [&calls, &viewModel](const QString& text)
    {
        ++calls.syncCurrentDraftText;
        viewModel.syncCurrentDraftText(text);
    };
    port.syncCurrentDialogPinned = [&calls, &viewModel](bool pinned)
    {
        ++calls.syncCurrentDialogPinned;
        viewModel.syncCurrentDialogPinned(pinned);
    };
    port.syncCurrentDialogMuted = [&calls, &viewModel](bool muted)
    {
        ++calls.syncCurrentDialogMuted;
        viewModel.syncCurrentDialogMuted(muted);
    };
    controller.setDialogRuntimePort(std::move(port));

    ChatDialogCommandDependencies dependencies;
    int draftReqId = 0;
    int pinReqId = 0;
    dependencies.dispatchPayload =
        [&draftReqId, &draftPayload, &pinReqId, &pinPayload](int reqId, const QByteArray& payload)
    {
        if (reqId == 1074)
        {
            pinReqId = reqId;
            pinPayload = payload;
            return;
        }
        draftReqId = reqId;
        draftPayload = payload;
    };
    dependencies.savePersistentDialogStore = [&savedOwnerUid](int ownerUid)
    {
        savedOwnerUid = ownerUid;
    };
    dependencies.refreshDialogModel = [&refreshCount]()
    {
        ++refreshCount;
    };

    ChatDialogCommandRequest request;
    request.selfUid = 1001;
    request.ownerUid = 1001;
    request.currentDialogUid = 2002;
    request.dialogUid = 2002;
    request.target.dialogType = QStringLiteral("private");
    request.target.peerUid = 2002;
    request.draftText = QStringLiteral("draft");

    ChatDialogCommandResult draftResult = controller.updateDialogDraft(request, dependencies);
    ASSERT_TRUE(draftResult.success);
    EXPECT_TRUE(draftResult.currentDraftUpdated);
    EXPECT_TRUE(draftResult.dialogModelUpdated);
    EXPECT_TRUE(draftResult.persistentStoreSaved);
    EXPECT_TRUE(draftResult.dispatched);
    EXPECT_EQ(calls.syncCurrentDraftText, 1);
    EXPECT_EQ(savedOwnerUid, 1001);
    EXPECT_EQ(viewModel.currentDraftText(), QStringLiteral("draft"));
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("draftText")).toString(),
                        QStringLiteral("draft"));
    EXPECT_EQ(draftReqId, 1072);
    EXPECT_EQ(compactJsonObject(draftPayload).value(QStringLiteral("draft_text")).toString(), QStringLiteral("draft"));

    ChatDialogCommandResult muteResult = controller.toggleDialogMuted(request, dependencies);
    ASSERT_TRUE(muteResult.success);
    EXPECT_TRUE(muteResult.currentMutedUpdated);
    EXPECT_TRUE(muteResult.dialogModelUpdated);
    EXPECT_TRUE(muteResult.dispatched);
    EXPECT_EQ(calls.syncCurrentDialogMuted, 1);
    EXPECT_TRUE(viewModel.currentDialogMuted());
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("muteState")).toInt(), 1);
    EXPECT_EQ(draftReqId, 1072);
    EXPECT_EQ(compactJsonObject(draftPayload).value(QStringLiteral("mute_state")).toInt(), 1);

    ChatDialogCommandResult pinResult = controller.toggleDialogPinned(request, dependencies);
    ASSERT_TRUE(pinResult.success);
    EXPECT_TRUE(pinResult.currentPinnedUpdated);
    EXPECT_TRUE(pinResult.persistentStoreSaved);
    EXPECT_TRUE(pinResult.dispatched);
    EXPECT_TRUE(pinResult.dialogModelRefreshed);
    EXPECT_EQ(calls.syncCurrentDialogPinned, 1);
    EXPECT_TRUE(viewModel.currentDialogPinned());
    EXPECT_EQ(refreshCount, 1);
    EXPECT_EQ(pinReqId, 1074);
    EXPECT_GT(compactJsonObject(pinPayload).value(QStringLiteral("pinned_rank")).toInt(), 0);
}

TEST(ChatFeatureControllerTest, MarkPrivateDialogReadOwnsUnreadAndReadAckEffects)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    auto friendInfo = std::make_shared<FriendInfo>(
        2002,
        QStringLiteral("Alice"), QStringLiteral("Alice"), QStringLiteral("alice.png"), 0, QString(), QString());
    friendInfo->_dialog_type = QStringLiteral("private");
    friendInfo->_unread_count = 3;
    controller.dialogListModel().upsertFriend(friendInfo);
    controller.chatListModel().upsertFriend(friendInfo);

    int dispatchedReqId = 0;
    QByteArray dispatchedPayload;
    ChatMarkDialogReadDependencies dependencies;
    dependencies.readAckDispatchPort.dispatchPayload =
        [&dispatchedReqId, &dispatchedPayload](int reqId, const QByteArray& payload)
    {
        dispatchedReqId = reqId;
        dispatchedPayload = payload;
    };
    ChatMarkDialogReadRequest request;
    request.selfUid = 1001;
    request.dialogUid = 2002;
    request.target.dialogType = QStringLiteral("private");
    request.target.peerUid = 2002;
    request.latestPrivatePeerTs = 7777;

    const ChatMarkDialogReadResult result = controller.markDialogRead(request, std::move(dependencies));

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.dialogUnreadCleared);
    EXPECT_TRUE(result.privateUnreadCleared);
    EXPECT_TRUE(result.privateReadAckDispatched);
    EXPECT_EQ(result.readAckTs, 7777);
    EXPECT_EQ(dispatchedReqId, 1076);
    EXPECT_EQ(listValue(controller.dialogListModel(), 2002, QStringLiteral("unreadCount")).toInt(), 0);
    EXPECT_EQ(listValue(controller.chatListModel(), 2002, QStringLiteral("unreadCount")).toInt(), 0);
    const QJsonObject payload = compactJsonObject(dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), 2002);
    EXPECT_EQ(payload.value(QStringLiteral("read_ts")).toVariant().toLongLong(), 7777);
}

TEST(ChatFeatureControllerTest, MarkGroupDialogReadOwnsMentionAndReadAckEffects)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    FriendListModel dialogListModel;
    FriendListModel groupListModel;
    const int dialogUid = ConversationSyncService::makeGroupDialogUid(7001);
    auto groupInfo = std::make_shared<FriendInfo>(
        dialogUid,
        QStringLiteral("Team"), QStringLiteral("Team"), QStringLiteral("team.png"), 0, QString(), QString());
    groupInfo->_dialog_type = QStringLiteral("group");
    groupInfo->_unread_count = 4;
    groupInfo->_mention_count = 2;
    dialogListModel.upsertFriend(groupInfo);
    groupListModel.upsertFriend(groupInfo);
    controller.setMentionCountForDialog(dialogUid, 2);

    int dispatchedReqId = 0;
    QByteArray dispatchedPayload;
    ChatMarkDialogReadDependencies dependencies;
    dependencies.dialogListModel = &dialogListModel;
    dependencies.groupListModel = &groupListModel;
    dependencies.readAckDispatchPort.dispatchPayload =
        [&dispatchedReqId, &dispatchedPayload](int reqId, const QByteArray& payload)
    {
        dispatchedReqId = reqId;
        dispatchedPayload = payload;
    };
    ChatMarkDialogReadRequest request;
    request.selfUid = 1001;
    request.dialogUid = dialogUid;
    request.target.dialogType = QStringLiteral("group");
    request.target.groupId = 7001;
    request.latestGroupTs = 8888;

    const ChatMarkDialogReadResult result = controller.markDialogRead(request, std::move(dependencies));

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.dialogUnreadCleared);
    EXPECT_TRUE(result.mentionCleared);
    EXPECT_TRUE(result.groupUnreadCleared);
    EXPECT_TRUE(result.groupReadAckDispatched);
    EXPECT_EQ(result.readAckTs, 8888);
    EXPECT_EQ(dispatchedReqId, 1071);
    EXPECT_EQ(listValue(dialogListModel, dialogUid, QStringLiteral("unreadCount")).toInt(), 0);
    EXPECT_EQ(listValue(dialogListModel, dialogUid, QStringLiteral("mentionCount")).toInt(), 0);
    EXPECT_EQ(listValue(groupListModel, dialogUid, QStringLiteral("unreadCount")).toInt(), 0);
    EXPECT_EQ(controller.dialogDecorationState().mentionMap->value(dialogUid, 0), 0);
    const QJsonObject payload = compactJsonObject(dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
    EXPECT_EQ(payload.value(QStringLiteral("read_ts")).toVariant().toLongLong(), 8888);
}

TEST(ChatFeatureControllerTest, MarkDialogReadByUidDerivesPrivateWatermarkFromPeerMessages)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    auto friendInfo = std::make_shared<FriendInfo>(
        2002,
        QStringLiteral("Alice"), QStringLiteral("Alice"), QStringLiteral("alice.png"), 0, QString(), QString());
    friendInfo->_dialog_type = QStringLiteral("private");
    friendInfo->_unread_count = 3;
    constexpr qint64 selfNewerTs = 1710000009999;
    constexpr qint64 peerOldTs = 1710000007777;
    constexpr qint64 peerNewTs = 1710000008888;
    friendInfo->_chat_msgs.push_back(std::make_shared<TextChatData>(
        QStringLiteral("self-newer"), QStringLiteral("self"), 1001, 2002, QStringLiteral("Me"), selfNewerTs));
    friendInfo->_chat_msgs.push_back(std::make_shared<TextChatData>(
        QStringLiteral("peer-old"), QStringLiteral("old"), 2002, 1001, QStringLiteral("Alice"), peerOldTs));
    friendInfo->_chat_msgs.push_back(std::make_shared<TextChatData>(
        QStringLiteral("peer-new"), QStringLiteral("new"), 2002, 1001, QStringLiteral("Alice"), peerNewTs));
    controller.dialogListModel().upsertFriend(friendInfo);
    controller.chatListModel().upsertFriend(friendInfo);

    int dispatchedReqId = 0;
    QByteArray dispatchedPayload;
    ChatDialogCommandPort commandPort;
    commandPort.snapshot = []
    {
        ChatDialogCommandSnapshot snapshot;
        snapshot.selfUid = 1001;
        return snapshot;
    };
    commandPort.privatePeerById = [friendInfo](int peerUid)
    {
        return peerUid == friendInfo->_uid ? friendInfo : nullptr;
    };
    controller.setDialogCommandPort(std::move(commandPort));
    ChatReadAckPort readAckPort;
    readAckPort.dispatch.dispatchPayload = [&dispatchedReqId, &dispatchedPayload](int reqId, const QByteArray& payload)
    {
        dispatchedReqId = reqId;
        dispatchedPayload = payload;
    };
    controller.setReadAckPort(std::move(readAckPort));

    const ChatMarkDialogReadResult result = controller.markDialogReadByUid(2002);

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.dialogUnreadCleared);
    EXPECT_TRUE(result.privateUnreadCleared);
    EXPECT_TRUE(result.privateReadAckDispatched);
    EXPECT_EQ(result.readAckTs, peerNewTs);
    EXPECT_EQ(dispatchedReqId, 1076);
    const QJsonObject payload = compactJsonObject(dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), 2002);
    EXPECT_EQ(payload.value(QStringLiteral("read_ts")).toVariant().toLongLong(), peerNewTs);
}

TEST(ChatFeatureControllerTest, MarkDialogReadByUidDerivesGroupWatermarkFromGroupMessages)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    FriendListModel groupListModel;
    const qint64 groupId = 7001;
    const int dialogUid = ConversationSyncService::makeGroupDialogUid(groupId);
    auto groupDialog = std::make_shared<FriendInfo>(
        dialogUid,
        QStringLiteral("Team"), QStringLiteral("Team"), QStringLiteral("team.png"), 0, QString(), QString());
    groupDialog->_dialog_type = QStringLiteral("group");
    groupDialog->_unread_count = 4;
    controller.dialogListModel().upsertFriend(groupDialog);
    groupListModel.upsertFriend(groupDialog);
    auto groupInfo = std::make_shared<GroupInfoData>();
    groupInfo->_group_id = groupId;
    constexpr qint64 groupOldTs = 1710000006666;
    constexpr qint64 groupNewTs = 1710000009999;
    groupInfo->_chat_msgs.push_back(std::make_shared<TextChatData>(
        QStringLiteral("g-old"), QStringLiteral("old"), 3001, 0, QStringLiteral("A"), groupOldTs));
    groupInfo->_chat_msgs.push_back(std::make_shared<TextChatData>(
        QStringLiteral("g-new"), QStringLiteral("new"), 3002, 0, QStringLiteral("B"), groupNewTs));

    int dispatchedReqId = 0;
    QByteArray dispatchedPayload;
    ChatDialogCommandPort commandPort;
    commandPort.snapshot = []
    {
        ChatDialogCommandSnapshot snapshot;
        snapshot.selfUid = 1001;
        return snapshot;
    };
    commandPort.targetForDialogUid = [groupId, dialogUid](int uid)
    {
        ChatDialogTarget target;
        if (uid == dialogUid)
        {
            target.dialogType = QStringLiteral("group");
            target.groupId = groupId;
        }
        return target;
    };
    commandPort.groupById = [groupInfo](qint64 requestedGroupId)
    {
        return requestedGroupId == groupInfo->_group_id ? groupInfo : nullptr;
    };
    commandPort.groupListModel = &groupListModel;
    controller.setDialogCommandPort(std::move(commandPort));
    ChatReadAckPort readAckPort;
    readAckPort.dispatch.dispatchPayload = [&dispatchedReqId, &dispatchedPayload](int reqId, const QByteArray& payload)
    {
        dispatchedReqId = reqId;
        dispatchedPayload = payload;
    };
    controller.setReadAckPort(std::move(readAckPort));

    const ChatMarkDialogReadResult result = controller.markDialogReadByUid(dialogUid);

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.dialogUnreadCleared);
    EXPECT_TRUE(result.groupUnreadCleared);
    EXPECT_TRUE(result.groupReadAckDispatched);
    EXPECT_EQ(result.readAckTs, groupNewTs);
    EXPECT_EQ(dispatchedReqId, 1071);
    EXPECT_EQ(listValue(groupListModel, dialogUid, QStringLiteral("unreadCount")).toInt(), 0);
    const QJsonObject payload = compactJsonObject(dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), groupId);
    EXPECT_EQ(payload.value(QStringLiteral("read_ts")).toVariant().toLongLong(), groupNewTs);
}

TEST(ChatFeatureControllerTest, DirectReadAckCommandsOwnPayloadAndRequestIds)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    int privateDispatchCount = 0;
    int privateRequestId = 0;
    QByteArray privatePayload;
    ChatReadAckDispatchPort privatePort;
    privatePort.dispatchPayload =
        [&privateDispatchCount, &privateRequestId, &privatePayload](int reqId, const QByteArray& payload)
    {
        ++privateDispatchCount;
        privateRequestId = reqId;
        privatePayload = payload;
    };
    ChatReadAckCommand privateRequest;
    privateRequest.selfUid = 1001;
    privateRequest.peerUid = 2002;
    privateRequest.readTs = 7777;

    const ChatReadAckCommandResult privateResult = controller.sendPrivateReadAck(privateRequest, privatePort);

    ASSERT_TRUE(privateResult.success);
    EXPECT_FALSE(privateResult.skipped);
    EXPECT_TRUE(privateResult.privatePath);
    EXPECT_FALSE(privateResult.groupPath);
    EXPECT_TRUE(privateResult.dispatched);
    EXPECT_EQ(privateResult.requestId, 1076);
    EXPECT_EQ(privateDispatchCount, 1);
    EXPECT_EQ(privateRequestId, 1076);
    const QJsonObject privateJson = compactJsonObject(privatePayload);
    EXPECT_EQ(privateJson.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(privateJson.value(QStringLiteral("peer_uid")).toInt(), 2002);
    EXPECT_EQ(privateJson.value(QStringLiteral("read_ts")).toVariant().toLongLong(), 7777);

    int groupDispatchCount = 0;
    int groupRequestId = 0;
    QByteArray groupPayload;
    ChatReadAckDispatchPort groupPort;
    groupPort.dispatchPayload =
        [&groupDispatchCount, &groupRequestId, &groupPayload](int reqId, const QByteArray& payload)
    {
        ++groupDispatchCount;
        groupRequestId = reqId;
        groupPayload = payload;
    };
    ChatReadAckCommand groupRequest;
    groupRequest.selfUid = 1001;
    groupRequest.groupId = 7001;
    groupRequest.readTs = 8888;

    const ChatReadAckCommandResult groupResult = controller.sendGroupReadAck(groupRequest, groupPort);

    ASSERT_TRUE(groupResult.success);
    EXPECT_FALSE(groupResult.skipped);
    EXPECT_FALSE(groupResult.privatePath);
    EXPECT_TRUE(groupResult.groupPath);
    EXPECT_TRUE(groupResult.dispatched);
    EXPECT_EQ(groupResult.requestId, 1071);
    EXPECT_EQ(groupDispatchCount, 1);
    EXPECT_EQ(groupRequestId, 1071);
    const QJsonObject groupJson = compactJsonObject(groupPayload);
    EXPECT_EQ(groupJson.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(groupJson.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
    EXPECT_EQ(groupJson.value(QStringLiteral("read_ts")).toVariant().toLongLong(), 8888);
}

TEST(ChatFeatureControllerTest, ReadAckFacadeUsesStoredFeaturePort)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    int selfUidCallCount = 0;
    int dispatchCount = 0;
    int lastReqId = 0;
    QVector<QByteArray> payloads;
    ChatReadAckPort port;
    port.selfUid = [&selfUidCallCount]()
    {
        ++selfUidCallCount;
        return 1001;
    };
    port.dispatch.dispatchPayload = [&dispatchCount, &lastReqId, &payloads](int reqId, const QByteArray& payload)
    {
        ++dispatchCount;
        lastReqId = reqId;
        payloads.push_back(payload);
    };
    controller.setReadAckPort(std::move(port));

    const ChatReadAckCommandResult privateResult = controller.sendPrivateReadAckForPeer(2002, 7777);
    ASSERT_TRUE(privateResult.success);
    EXPECT_TRUE(privateResult.privatePath);
    EXPECT_TRUE(privateResult.dispatched);
    EXPECT_EQ(privateResult.selfUid, 1001);
    EXPECT_EQ(privateResult.peerUid, 2002);
    EXPECT_EQ(privateResult.requestId, 1076);
    EXPECT_EQ(lastReqId, 1076);

    const ChatReadAckCommandResult groupResult = controller.sendGroupReadAckForGroup(7001, 8888);
    ASSERT_TRUE(groupResult.success);
    EXPECT_TRUE(groupResult.groupPath);
    EXPECT_TRUE(groupResult.dispatched);
    EXPECT_EQ(groupResult.selfUid, 1001);
    EXPECT_EQ(groupResult.groupId, 7001);
    EXPECT_EQ(groupResult.requestId, 1071);
    EXPECT_EQ(lastReqId, 1071);

    EXPECT_EQ(selfUidCallCount, 2);
    EXPECT_EQ(dispatchCount, 2);
    ASSERT_EQ(payloads.size(), 2);
    const QJsonObject privateJson = compactJsonObject(payloads[0]);
    EXPECT_EQ(privateJson.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(privateJson.value(QStringLiteral("peer_uid")).toInt(), 2002);
    const QJsonObject groupJson = compactJsonObject(payloads[1]);
    EXPECT_EQ(groupJson.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(groupJson.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
}

TEST(ChatFeatureControllerTest, DirectReadAckCommandsSkipInvalidInputsBeforeDispatch)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    int dispatchCount = 0;
    ChatReadAckDispatchPort port;
    port.dispatchPayload = [&dispatchCount](int, const QByteArray&)
    {
        ++dispatchCount;
    };

    ChatReadAckCommand invalidPrivate;
    invalidPrivate.selfUid = 1001;
    invalidPrivate.peerUid = 0;
    const ChatReadAckCommandResult privateResult = controller.sendPrivateReadAck(invalidPrivate, port);

    EXPECT_FALSE(privateResult.success);
    EXPECT_TRUE(privateResult.skipped);
    EXPECT_TRUE(privateResult.privatePath);
    EXPECT_FALSE(privateResult.dispatched);
    EXPECT_TRUE(privateResult.compactPayload.isEmpty());

    ChatReadAckCommand invalidGroup;
    invalidGroup.selfUid = 0;
    invalidGroup.groupId = 7001;
    const ChatReadAckCommandResult groupResult = controller.sendGroupReadAck(invalidGroup, port);

    EXPECT_FALSE(groupResult.success);
    EXPECT_TRUE(groupResult.skipped);
    EXPECT_TRUE(groupResult.groupPath);
    EXPECT_FALSE(groupResult.dispatched);
    EXPECT_TRUE(groupResult.compactPayload.isEmpty());
    EXPECT_EQ(dispatchCount, 0);
}

TEST(ChatFeatureControllerTest, DialogRuntimeReplyContextIsNormalizedAndChangeTracked)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    const QString longPreview(130, QLatin1Char('p'));
    EXPECT_TRUE(controller.setPendingReplyContext(
        QStringLiteral(" msg-1 "), QStringLiteral(" Alice "), QStringLiteral(" ") + longPreview));
    EXPECT_TRUE(controller.hasPendingReply());
    EXPECT_EQ(controller.replyToMsgId(), QStringLiteral("msg-1"));
    EXPECT_EQ(controller.replyTargetName(), QStringLiteral("Alice"));
    EXPECT_EQ(controller.replyPreviewText().size(), 120);

    EXPECT_FALSE(controller.setPendingReplyContext(QStringLiteral("msg-1"), QStringLiteral("Alice"), longPreview));
    EXPECT_TRUE(controller.setPendingReplyContext(QString(), QString(), QString()));
    EXPECT_FALSE(controller.hasPendingReply());
    EXPECT_TRUE(controller.replyToMsgId().isEmpty());
    EXPECT_TRUE(controller.replyTargetName().isEmpty());
    EXPECT_TRUE(controller.replyPreviewText().isEmpty());
}

TEST(ChatFeatureControllerTest, PendingAttachmentsNormalizeInvalidInputAndRemoveById)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    QVariantMap invalidType;
    invalidType.insert(QStringLiteral("type"), QStringLiteral("video"));
    invalidType.insert(QStringLiteral("fileUrl"), QStringLiteral("file:///tmp/video.mp4"));

    QVariantMap missingUrl;
    missingUrl.insert(QStringLiteral("type"), QStringLiteral("image"));

    QVariantMap validImage;
    validImage.insert(QStringLiteral("type"), QStringLiteral(" image "));
    validImage.insert(QStringLiteral("fileUrl"), QStringLiteral("file:///tmp/photo.png"));

    const ChatPendingAttachmentCommandResult addResult =
        controller.addPendingAttachmentsForDialog(42, QVariantList{invalidType, missingUrl, validImage});

    ASSERT_TRUE(addResult.valid);
    ASSERT_TRUE(addResult.changed);
    ASSERT_EQ(addResult.attachments.size(), 1);
    const QVariantMap storedAttachment = addResult.attachments.first().toMap();
    EXPECT_EQ(storedAttachment.value(QStringLiteral("type")).toString(), QStringLiteral("image"));
    EXPECT_EQ(storedAttachment.value(QStringLiteral("fileUrl")).toString(), QStringLiteral("file:///tmp/photo.png"));
    EXPECT_FALSE(storedAttachment.value(QStringLiteral("attachmentId")).toString().isEmpty());
    EXPECT_EQ(storedAttachment.value(QStringLiteral("fileName")).toString(), QStringLiteral("photo.png"));
    EXPECT_EQ(controller.pendingAttachmentsForDialog(42), addResult.attachments);

    const ChatPendingAttachmentCommandResult missingRemove =
        controller.removePendingAttachmentByIdForDialog(42, QStringLiteral("missing"));
    EXPECT_TRUE(missingRemove.valid);
    EXPECT_FALSE(missingRemove.changed);
    EXPECT_EQ(controller.pendingAttachmentsForDialog(42), addResult.attachments);

    const ChatPendingAttachmentCommandResult removeResult = controller.removePendingAttachmentByIdForDialog(
        42,
        storedAttachment.value(QStringLiteral("attachmentId")).toString());
    ASSERT_TRUE(removeResult.valid);
    ASSERT_TRUE(removeResult.changed);
    EXPECT_TRUE(removeResult.attachments.isEmpty());
    EXPECT_TRUE(controller.pendingAttachmentsForDialog(42).isEmpty());
}

TEST(ChatFeatureControllerTest, PendingSendQueueBeginsSnapshotsAdvancesAndResets)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);

    QVariantMap firstAttachment;
    firstAttachment.insert(QStringLiteral("attachmentId"), QStringLiteral("att-1"));
    firstAttachment.insert(QStringLiteral("type"), QStringLiteral("image"));
    firstAttachment.insert(QStringLiteral("fileUrl"), QStringLiteral("file:///tmp/a.png"));

    QVariantMap secondAttachment;
    secondAttachment.insert(QStringLiteral("attachmentId"), QStringLiteral("att-2"));
    secondAttachment.insert(QStringLiteral("type"), QStringLiteral("file"));
    secondAttachment.insert(QStringLiteral("fileUrl"), QStringLiteral("file:///tmp/b.txt"));

    EXPECT_FALSE(controller.beginPendingAttachmentSend(0, 2002, 0, QVariantList{firstAttachment}));
    EXPECT_TRUE(controller.pendingAttachmentSendQueueEmpty());

    ASSERT_TRUE(controller.beginPendingAttachmentSend(42, 2002, 0, QVariantList{firstAttachment, secondAttachment}));
    EXPECT_FALSE(controller.pendingAttachmentSendQueueEmpty());
    EXPECT_EQ(controller.pendingAttachmentSendDialogUid(), 42);
    EXPECT_EQ(controller.pendingAttachmentSendChatUid(), 2002);
    EXPECT_EQ(controller.pendingAttachmentSendGroupId(), 0);
    EXPECT_EQ(controller.pendingAttachmentSendTotalCount(), 2);
    EXPECT_EQ(controller.pendingAttachmentSendRemainingCount(), 2);
    EXPECT_EQ(controller.pendingAttachmentSendCurrentIndex(), 1);
    EXPECT_EQ(controller.currentPendingAttachmentToSend().value(QStringLiteral("attachmentId")).toString(),
                                                                QStringLiteral("att-1"));

    ChatPendingSendQueueSnapshot snapshot = controller.pendingAttachmentSendSnapshot();
    ASSERT_TRUE(snapshot.hasCurrent);
    EXPECT_EQ(snapshot.dialogUid, 42);
    EXPECT_EQ(snapshot.chatUid, 2002);
    EXPECT_EQ(snapshot.groupId, 0);
    EXPECT_EQ(snapshot.totalCount, 2);
    EXPECT_EQ(snapshot.remainingCount, 2);
    EXPECT_EQ(snapshot.currentIndex, 1);
    EXPECT_EQ(snapshot.currentAttachment.value(QStringLiteral("attachmentId")).toString(), QStringLiteral("att-1"));

    EXPECT_TRUE(controller.advancePendingAttachmentSendQueue());
    EXPECT_EQ(controller.pendingAttachmentSendRemainingCount(), 1);
    EXPECT_EQ(controller.pendingAttachmentSendCurrentIndex(), 2);
    EXPECT_EQ(controller.currentPendingAttachmentToSend().value(QStringLiteral("attachmentId")).toString(),
                                                                QStringLiteral("att-2"));

    EXPECT_FALSE(controller.advancePendingAttachmentSendQueue());
    snapshot = controller.pendingAttachmentSendSnapshot();
    EXPECT_FALSE(snapshot.hasCurrent);
    EXPECT_TRUE(controller.pendingAttachmentSendQueueEmpty());
    EXPECT_EQ(controller.pendingAttachmentSendTotalCount(), 0);
    EXPECT_EQ(controller.pendingAttachmentSendRemainingCount(), 0);
    EXPECT_EQ(controller.pendingAttachmentSendCurrentIndex(), 0);

    ASSERT_TRUE(controller.beginSinglePendingAttachmentSend(99, 0, 7001, firstAttachment));
    snapshot = controller.pendingAttachmentSendSnapshot();
    ASSERT_TRUE(snapshot.hasCurrent);
    EXPECT_EQ(snapshot.dialogUid, 99);
    EXPECT_EQ(snapshot.chatUid, 0);
    EXPECT_EQ(snapshot.groupId, 7001);
    EXPECT_EQ(snapshot.totalCount, 1);
    EXPECT_EQ(snapshot.remainingCount, 1);

    controller.resetPendingAttachmentSendQueue();
    EXPECT_TRUE(controller.pendingAttachmentSendQueueEmpty());
    EXPECT_FALSE(controller.pendingAttachmentSendSnapshot().hasCurrent);
}

TEST(ChatFeatureControllerTest, GroupTextSendOwnsOptimisticStateBeforeDispatch)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;

    GroupSendRequest request;
    request.context = harness.context();
    request.groupId = 7001;
    request.content = QStringLiteral("hello @u000001001");

    QString pendingGroupDuringDispatch;
    auto deps = harness.dependencies();
    deps.dispatchPayload = [&harness, &controller, &pendingGroupDuringDispatch](int reqId, const QByteArray& payload)
    {
        harness.lastChatRequestId = reqId;
        harness.lastChatPayload = payload;
        ++harness.chatDispatchCount;
        const QString msgId = compactJsonObject(payload)
                                  .value(QStringLiteral("msg")).toObject().value(QStringLiteral("msgid")).toString();
        pendingGroupDuringDispatch =
            QString::number(controller.groupConversationState().pendingMsgGroupMap.value(msgId));
    };

    const GroupSendResult result = controller.sendGroupText(request, deps);

    ASSERT_TRUE(result.success);
    EXPECT_EQ(harness.chatDispatchCount, 1);
    EXPECT_EQ(harness.lastChatRequestId, 1044);
    EXPECT_EQ(pendingGroupDuringDispatch, QStringLiteral("7001"));
    EXPECT_EQ(controller.groupConversationState().pendingMsgGroupMap.value(result.msgId), 7001);
    EXPECT_EQ(harness.groups.value(7001)->_chat_msgs.size(), 1U);
    EXPECT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(modelValue(harness.messageModel, 0, ChatMessageModel::RawContentRole).toString(), request.content);
    EXPECT_EQ(modelValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("sending"));

    const int dialogUid = harness.dialogUidFor(7001);
    EXPECT_EQ(listValue(harness.dialogListModel, dialogUid, QStringLiteral("lastMsg")).toString(),
                        QStringLiteral("hello @u000001001"));
    EXPECT_EQ(listValue(harness.dialogListModel, dialogUid, QStringLiteral("unreadCount")).toInt(), 0);

    const QJsonObject payload = compactJsonObject(harness.lastChatPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
    EXPECT_EQ(
        payload.value(QStringLiteral("msg")).toObject().value(QStringLiteral("content")).toString(), request.content);
}

TEST(ChatFeatureControllerTest, GroupHistoryRequestAndResponseOwnCursorAndCurrentModel)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;
    auto group = harness.groups.value(7001);
    group->_chat_msgs.push_back(std::make_shared<TextChatData>(QStringLiteral("old"),
        QStringLiteral("old text"),
            2002,
            0,
            QStringLiteral("Alice"), 1700000001000LL, QStringLiteral("a.png"), QStringLiteral("sent"), 10, 22));

    GroupHistoryBuildRequest buildRequest;
    buildRequest.selfUid = 1001;
    buildRequest.groupId = 7001;
    buildRequest.limit = 25;
    const GroupHistoryBuildResult buildResult =
        controller.buildGroupHistoryRequest(buildRequest, harness.dependencies());

    ASSERT_TRUE(buildResult.success);
    EXPECT_EQ(harness.historyDispatchCount, 1);
    EXPECT_EQ(harness.lastHistoryRequestId, 1047);
    const QJsonObject historyPayload = compactJsonObject(harness.lastHistoryPayload);
    EXPECT_EQ(historyPayload.value(QStringLiteral("limit")).toInt(), 25);
    EXPECT_EQ(historyPayload.value(QStringLiteral("before_seq")).toVariant().toLongLong(), 0);

    QJsonObject responsePayload;
    responsePayload.insert(QStringLiteral("groupid"), static_cast<qint64>(7001));
    responsePayload.insert(QStringLiteral("has_more"), true);
    responsePayload.insert(QStringLiteral("next_before_seq"), static_cast<qint64>(21));
    GroupHistoryResponseRequest responseRequest;
    responseRequest.context = harness.context();
    responseRequest.payload = responsePayload;
    int groupLoadingProjectionCount = 0;
    int privateLoadingProjectionCount = 0;
    int canLoadMoreProjectionCount = 0;
    int readAckRequestCount = 0;
    int statusSetCount = 0;
    bool groupLoadingProjected = true;
    bool privateLoadingProjected = true;
    bool canLoadMoreProjected = false;
    qint64 readAckGroupId = 0;
    qint64 readAckTs = 0;
    QString statusText;
    bool statusIsError = true;
    GroupHistoryResponsePort responsePort;
    responsePort.setGroupHistoryLoading = [&groupLoadingProjectionCount, &groupLoadingProjected](bool loading)
    {
        ++groupLoadingProjectionCount;
        groupLoadingProjected = loading;
    };
    responsePort.setPrivateHistoryLoading = [&privateLoadingProjectionCount, &privateLoadingProjected](bool loading)
    {
        ++privateLoadingProjectionCount;
        privateLoadingProjected = loading;
    };
    responsePort.setCanLoadMorePrivateHistory = [&canLoadMoreProjectionCount, &canLoadMoreProjected](bool canLoad)
    {
        ++canLoadMoreProjectionCount;
        canLoadMoreProjected = canLoad;
    };
    responsePort.sendGroupReadAck = [&readAckRequestCount, &readAckGroupId, &readAckTs](qint64 groupId, qint64 readTs)
    {
        ++readAckRequestCount;
        readAckGroupId = groupId;
        readAckTs = readTs;
    };
    responsePort.setGroupStatus = [&statusSetCount, &statusText, &statusIsError](const QString& text, bool isError)
    {
        ++statusSetCount;
        statusText = text;
        statusIsError = isError;
    };
    const GroupHistoryResponseResult responseResult =
        controller.handleGroupHistoryResponse(responseRequest, harness.dependencies(), responsePort);

    ASSERT_TRUE(responseResult.success);
    EXPECT_TRUE(responseResult.currentDialog);
    EXPECT_TRUE(responseResult.modelSet);
    EXPECT_TRUE(responseResult.loadingCleared);
    EXPECT_TRUE(responseResult.canLoadMoreProjected);
    EXPECT_TRUE(responseResult.readAckRequested);
    EXPECT_TRUE(responseResult.statusSet);
    EXPECT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(controller.groupConversationState().historyBeforeSeq, 21);
    EXPECT_TRUE(controller.groupConversationState().historyHasMore);
    EXPECT_EQ(responseResult.requestedReadAckTs, 1700000001000LL);
    EXPECT_EQ(responseResult.currentGroupId, 7001);
    EXPECT_EQ(responseResult.cachedMessageCount, 1);
    EXPECT_EQ(groupLoadingProjectionCount, 1);
    EXPECT_FALSE(groupLoadingProjected);
    EXPECT_EQ(privateLoadingProjectionCount, 1);
    EXPECT_FALSE(privateLoadingProjected);
    EXPECT_EQ(canLoadMoreProjectionCount, 1);
    EXPECT_TRUE(canLoadMoreProjected);
    EXPECT_EQ(readAckRequestCount, 1);
    EXPECT_EQ(readAckGroupId, 7001);
    EXPECT_EQ(readAckTs, 1700000001000LL);
    EXPECT_EQ(statusSetCount, 1);
    EXPECT_EQ(statusText, QStringLiteral("历史消息已加载"));
    EXPECT_FALSE(statusIsError);
}

TEST(ChatFeatureControllerTest, GroupConversationClearRemovesBackgroundGroupWithoutCurrentReset)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;
    const int dialogUid = harness.dialogUidFor(7001);
    controller.dialogListModel().upsertFriend(
        makeListDialog(dialogUid, QStringLiteral("Team"), QStringLiteral("group")));
    controller.setMentionCountForDialog(dialogUid, 3);

    int removedGroupDialogUid = 0;
    int removedMappingDialogUid = 0;
    int currentResetCalls = 0;
    int messageClearCalls = 0;

    ChatGroupConversationClearPort port;
    port.dialogUidForGroup = [&harness](qint64 groupId)
    {
        return harness.dialogUidFor(groupId);
    };
    port.removeGroupByDialogUid = [&removedGroupDialogUid](int value)
    {
        removedGroupDialogUid = value;
    };
    port.removeGroupDialogMapping = [&removedMappingDialogUid](int value)
    {
        removedMappingDialogUid = value;
    };
    port.clearCurrentGroup = [&currentResetCalls]()
    {
        ++currentResetCalls;
    };
    port.clearMessageModel = [&messageClearCalls]()
    {
        ++messageClearCalls;
    };

    ChatGroupConversationClearRequest request;
    request.groupId = 7001;
    request.currentGroupId = 8001;
    const ChatGroupConversationClearResult result = controller.clearGroupConversation(request, port);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.targetGroupId, 7001);
    EXPECT_EQ(result.dialogUid, dialogUid);
    EXPECT_TRUE(result.dialogRemoved);
    EXPECT_TRUE(result.groupRemoved);
    EXPECT_TRUE(result.mappingRemoved);
    EXPECT_FALSE(result.currentConversationCleared);
    EXPECT_EQ(controller.dialogListModel().indexOfUid(dialogUid), -1);
    EXPECT_EQ(removedGroupDialogUid, dialogUid);
    EXPECT_EQ(removedMappingDialogUid, dialogUid);
    EXPECT_EQ(currentResetCalls, 0);
    EXPECT_EQ(messageClearCalls, 0);
}

TEST(ChatFeatureControllerTest, GroupConversationClearResetsCurrentConversationEffects)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;
    const int dialogUid = harness.dialogUidFor(7001);
    controller.dialogListModel().upsertFriend(
        makeListDialog(dialogUid, QStringLiteral("Team"), QStringLiteral("group")));
    controller.groupConversationState().historyBeforeSeq = 99;
    controller.groupConversationState().historyHasMore = false;
    EXPECT_TRUE(controller.setCurrentDraftText(QStringLiteral("draft")));
    EXPECT_TRUE(controller.setCurrentDialogPinned(true));
    EXPECT_TRUE(controller.setCurrentDialogMuted(true));
    EXPECT_TRUE(
        controller.setPendingReplyContext(QStringLiteral("m1"), QStringLiteral("Alice"), QStringLiteral("preview")));

    int clearCurrentGroupCalls = 0;
    int groupLoadingFalseCalls = 0;
    int canLoadMoreFalseCalls = 0;
    int messageClearCalls = 0;
    int peerResetCalls = 0;
    int replyResetCalls = 0;
    int draftResetCalls = 0;
    int pinnedResetCalls = 0;
    int mutedResetCalls = 0;
    int dialogUidSignalCalls = 0;

    ChatGroupConversationClearPort port;
    port.dialogUidForGroup = [&harness](qint64 groupId)
    {
        return harness.dialogUidFor(groupId);
    };
    port.clearCurrentGroup = [&clearCurrentGroupCalls]()
    {
        ++clearCurrentGroupCalls;
    };
    port.setGroupHistoryLoading = [&groupLoadingFalseCalls](bool loading)
    {
        if (!loading)
        {
            ++groupLoadingFalseCalls;
        }
    };
    port.setCanLoadMorePrivateHistory = [&canLoadMoreFalseCalls](bool canLoad)
    {
        if (!canLoad)
        {
            ++canLoadMoreFalseCalls;
        }
    };
    port.clearMessageModel = [&messageClearCalls]()
    {
        ++messageClearCalls;
    };
    port.resetCurrentPeer = [&peerResetCalls]()
    {
        ++peerResetCalls;
    };
    port.setPendingReplyContext =
        [&replyResetCalls](const QString& msgId, const QString& senderName, const QString& previewText)
    {
        if (msgId.isEmpty() && senderName.isEmpty() && previewText.isEmpty())
        {
            ++replyResetCalls;
        }
    };
    port.setCurrentDraftText = [&draftResetCalls](const QString& text)
    {
        if (text.isEmpty())
        {
            ++draftResetCalls;
        }
    };
    port.setCurrentDialogPinned = [&pinnedResetCalls](bool pinned)
    {
        if (!pinned)
        {
            ++pinnedResetCalls;
        }
    };
    port.setCurrentDialogMuted = [&mutedResetCalls](bool muted)
    {
        if (!muted)
        {
            ++mutedResetCalls;
        }
    };
    port.emitCurrentDialogUidChangedIfNeeded = [&dialogUidSignalCalls]()
    {
        ++dialogUidSignalCalls;
    };

    ChatGroupConversationClearRequest request;
    request.groupId = 7001;
    request.currentGroupId = 7001;
    const ChatGroupConversationClearResult result = controller.clearGroupConversation(request, port);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.currentConversationCleared);
    EXPECT_TRUE(result.currentRuntimeReset);
    EXPECT_EQ(controller.groupConversationState().historyBeforeSeq, 0);
    EXPECT_TRUE(controller.groupConversationState().historyHasMore);
    EXPECT_EQ(clearCurrentGroupCalls, 1);
    EXPECT_EQ(groupLoadingFalseCalls, 1);
    EXPECT_EQ(canLoadMoreFalseCalls, 1);
    EXPECT_EQ(messageClearCalls, 1);
    EXPECT_EQ(peerResetCalls, 1);
    EXPECT_EQ(replyResetCalls, 1);
    EXPECT_EQ(draftResetCalls, 1);
    EXPECT_EQ(pinnedResetCalls, 1);
    EXPECT_EQ(mutedResetCalls, 1);
    EXPECT_EQ(dialogUidSignalCalls, 1);
}

TEST(ChatFeatureControllerTest, BackgroundGroupHistoryResponseDoesNotProjectCurrentEffects)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;
    auto group = harness.seedGroup(8001, QStringLiteral("Background"));
    group->_chat_msgs.push_back(std::make_shared<TextChatData>(QStringLiteral("bg"),
        QStringLiteral("background text"),
            2002,
            0,
            QStringLiteral("Alice"), 1700000002000LL, QStringLiteral("a.png"), QStringLiteral("sent"), 10, 30));

    int groupLoadingProjectionCount = 0;
    int privateLoadingProjectionCount = 0;
    int canLoadMoreProjectionCount = 0;
    int readAckRequestCount = 0;
    int statusSetCount = 0;
    bool canLoadMoreProjected = true;
    GroupHistoryResponsePort responsePort;
    responsePort.setGroupHistoryLoading = [&groupLoadingProjectionCount](bool)
    {
        ++groupLoadingProjectionCount;
    };
    responsePort.setPrivateHistoryLoading = [&privateLoadingProjectionCount](bool)
    {
        ++privateLoadingProjectionCount;
    };
    responsePort.setCanLoadMorePrivateHistory = [&canLoadMoreProjectionCount, &canLoadMoreProjected](bool canLoad)
    {
        ++canLoadMoreProjectionCount;
        canLoadMoreProjected = canLoad;
    };
    responsePort.sendGroupReadAck = [&readAckRequestCount](qint64, qint64)
    {
        ++readAckRequestCount;
    };
    responsePort.setGroupStatus = [&statusSetCount](const QString&, bool)
    {
        ++statusSetCount;
    };

    QJsonObject responsePayload;
    responsePayload.insert(QStringLiteral("groupid"), static_cast<qint64>(8001));
    responsePayload.insert(QStringLiteral("has_more"), true);
    GroupHistoryResponseRequest responseRequest;
    responseRequest.context = harness.context(7001);
    responseRequest.payload = responsePayload;
    const GroupHistoryResponseResult responseResult =
        controller.handleGroupHistoryResponse(responseRequest, harness.dependencies(), responsePort);

    ASSERT_TRUE(responseResult.success);
    EXPECT_FALSE(responseResult.currentDialog);
    EXPECT_FALSE(responseResult.loadingCleared);
    EXPECT_TRUE(responseResult.canLoadMoreProjected);
    EXPECT_FALSE(responseResult.readAckRequested);
    EXPECT_FALSE(responseResult.statusSet);
    EXPECT_EQ(responseResult.groupId, 8001);
    EXPECT_EQ(responseResult.currentGroupId, 7001);
    EXPECT_EQ(responseResult.cachedMessageCount, 1);
    EXPECT_EQ(groupLoadingProjectionCount, 0);
    EXPECT_EQ(privateLoadingProjectionCount, 0);
    EXPECT_EQ(canLoadMoreProjectionCount, 1);
    EXPECT_FALSE(canLoadMoreProjected);
    EXPECT_EQ(readAckRequestCount, 0);
    EXPECT_EQ(statusSetCount, 0);
}

TEST(ChatFeatureControllerTest, GroupHistoryCommandDispatchesAndProjectsLoading)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;
    controller.groupConversationState().historyBeforeSeq = 42;
    controller.groupConversationState().historyHasMore = true;

    int groupLoadingProjectionCount = 0;
    int privateLoadingProjectionCount = 0;
    bool groupLoadingProjected = false;
    bool privateLoadingProjected = false;

    GroupHistoryRequestCommand command;
    command.selfUid = 1001;
    command.currentGroupId = 7001;
    command.targetGroupId = 7001;
    command.currentGroupLoading = false;
    command.limit = 25;
    GroupHistoryRequestPort port;
    port.setGroupHistoryLoading = [&groupLoadingProjectionCount, &groupLoadingProjected](bool loading)
    {
        ++groupLoadingProjectionCount;
        groupLoadingProjected = loading;
    };
    port.setPrivateHistoryLoading = [&privateLoadingProjectionCount, &privateLoadingProjected](bool loading)
    {
        ++privateLoadingProjectionCount;
        privateLoadingProjected = loading;
    };

    const auto result = controller.requestGroupHistory(command, harness.dependencies(), port);

    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.skipped);
    EXPECT_TRUE(result.dispatched);
    EXPECT_TRUE(result.loadingProjected);
    EXPECT_EQ(harness.historyDispatchCount, 1);
    EXPECT_EQ(groupLoadingProjectionCount, 1);
    EXPECT_TRUE(groupLoadingProjected);
    EXPECT_EQ(privateLoadingProjectionCount, 1);
    EXPECT_TRUE(privateLoadingProjected);

    const QJsonObject payload = compactJsonObject(harness.lastHistoryPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
    EXPECT_EQ(payload.value(QStringLiteral("before_seq")).toVariant().toLongLong(), 42);
    EXPECT_EQ(payload.value(QStringLiteral("limit")).toInt(), 25);
}

TEST(ChatFeatureControllerTest, GroupHistoryCommandSkipsDuplicateCurrentLoading)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;

    int groupLoadingProjectionCount = 0;
    int privateLoadingProjectionCount = 0;

    GroupHistoryRequestCommand command;
    command.selfUid = 1001;
    command.currentGroupId = 7001;
    command.targetGroupId = 7001;
    command.currentGroupLoading = true;
    command.limit = 25;
    GroupHistoryRequestPort port;
    port.setGroupHistoryLoading = [&groupLoadingProjectionCount](bool)
    {
        ++groupLoadingProjectionCount;
    };
    port.setPrivateHistoryLoading = [&privateLoadingProjectionCount](bool)
    {
        ++privateLoadingProjectionCount;
    };

    const auto result = controller.requestGroupHistory(command, harness.dependencies(), port);

    EXPECT_TRUE(result.skipped);
    EXPECT_TRUE(result.skippedByLoading);
    EXPECT_FALSE(result.dispatched);
    EXPECT_FALSE(result.loadingProjected);
    EXPECT_EQ(harness.historyDispatchCount, 0);
    EXPECT_EQ(groupLoadingProjectionCount, 0);
    EXPECT_EQ(privateLoadingProjectionCount, 0);
}

TEST(ChatFeatureControllerTest, CurrentGroupHistoryFacadeBuildsRequestFromStoredPort)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;
    controller.groupConversationState().historyBeforeSeq = 77;

    int dependencyCalls = 0;
    int groupLoadingProjectionCount = 0;
    int privateLoadingProjectionCount = 0;
    bool groupLoadingProjected = false;
    bool privateLoadingProjected = false;

    ChatCurrentGroupHistoryPort port;
    port.snapshot = []
    {
        ChatCurrentGroupHistorySnapshot snapshot;
        snapshot.selfUid = 1001;
        snapshot.currentGroupId = 7001;
        snapshot.groupHistoryLoading = false;
        return snapshot;
    };
    port.dependencies = [&harness, &dependencyCalls]
    {
        ++dependencyCalls;
        return harness.dependencies();
    };
    port.requestPort.setGroupHistoryLoading = [&groupLoadingProjectionCount, &groupLoadingProjected](bool loading)
    {
        ++groupLoadingProjectionCount;
        groupLoadingProjected = loading;
    };
    port.requestPort.setPrivateHistoryLoading = [&privateLoadingProjectionCount, &privateLoadingProjected](bool loading)
    {
        ++privateLoadingProjectionCount;
        privateLoadingProjected = loading;
    };
    controller.setCurrentGroupHistoryPort(std::move(port));

    const auto result = controller.requestCurrentGroupHistory();

    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.skipped);
    EXPECT_TRUE(result.dispatched);
    EXPECT_EQ(dependencyCalls, 1);
    EXPECT_EQ(harness.historyDispatchCount, 1);
    EXPECT_EQ(groupLoadingProjectionCount, 1);
    EXPECT_TRUE(groupLoadingProjected);
    EXPECT_EQ(privateLoadingProjectionCount, 1);
    EXPECT_TRUE(privateLoadingProjected);

    const QJsonObject payload = compactJsonObject(harness.lastHistoryPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
    EXPECT_EQ(payload.value(QStringLiteral("before_seq")).toVariant().toLongLong(), 77);
    EXPECT_EQ(payload.value(QStringLiteral("limit")).toInt(), 50);
}

TEST(ChatFeatureControllerTest, GroupHistoryBootstrapOnlyProjectsCurrentGroupLoading)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;

    int groupLoadingProjectionCount = 0;
    int privateLoadingProjectionCount = 0;

    GroupHistoryRequestCommand backgroundCommand;
    backgroundCommand.selfUid = 1001;
    backgroundCommand.currentGroupId = 7001;
    backgroundCommand.targetGroupId = 8001;
    backgroundCommand.currentGroupLoading = true;
    backgroundCommand.limit = 30;
    GroupHistoryRequestPort backgroundPort;
    backgroundPort.setGroupHistoryLoading = [&groupLoadingProjectionCount](bool)
    {
        ++groupLoadingProjectionCount;
    };
    backgroundPort.setPrivateHistoryLoading = [&privateLoadingProjectionCount](bool)
    {
        ++privateLoadingProjectionCount;
    };

    const auto backgroundResult =
        controller.requestGroupHistoryForBootstrap(backgroundCommand, harness.dependencies(), backgroundPort);

    ASSERT_TRUE(backgroundResult.success);
    EXPECT_FALSE(backgroundResult.skipped);
    EXPECT_TRUE(backgroundResult.dispatched);
    EXPECT_FALSE(backgroundResult.loadingProjected);
    EXPECT_EQ(harness.historyDispatchCount, 1);
    EXPECT_EQ(groupLoadingProjectionCount, 0);
    EXPECT_EQ(privateLoadingProjectionCount, 0);
    QJsonObject payload = compactJsonObject(harness.lastHistoryPayload);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 8001);
    EXPECT_EQ(payload.value(QStringLiteral("before_seq")).toVariant().toLongLong(), 0);
    EXPECT_EQ(payload.value(QStringLiteral("limit")).toInt(), 30);

    GroupHistoryRequestCommand currentCommand;
    currentCommand.selfUid = 1001;
    currentCommand.currentGroupId = 7001;
    currentCommand.targetGroupId = 7001;
    currentCommand.currentGroupLoading = false;
    currentCommand.limit = 35;
    GroupHistoryRequestPort currentPort;
    currentPort.setGroupHistoryLoading = [&groupLoadingProjectionCount](bool loading)
    {
        ++groupLoadingProjectionCount;
        EXPECT_TRUE(loading);
    };
    currentPort.setPrivateHistoryLoading = [&privateLoadingProjectionCount](bool loading)
    {
        ++privateLoadingProjectionCount;
        EXPECT_TRUE(loading);
    };

    const auto currentResult =
        controller.requestGroupHistoryForBootstrap(currentCommand, harness.dependencies(), currentPort);

    ASSERT_TRUE(currentResult.success);
    EXPECT_FALSE(currentResult.skipped);
    EXPECT_TRUE(currentResult.dispatched);
    EXPECT_TRUE(currentResult.loadingProjected);
    EXPECT_EQ(harness.historyDispatchCount, 2);
    EXPECT_EQ(groupLoadingProjectionCount, 1);
    EXPECT_EQ(privateLoadingProjectionCount, 1);
    payload = compactJsonObject(harness.lastHistoryPayload);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
    EXPECT_EQ(payload.value(QStringLiteral("before_seq")).toVariant().toLongLong(), 0);
    EXPECT_EQ(payload.value(QStringLiteral("limit")).toInt(), 35);
}

TEST(ChatFeatureControllerTest, CurrentHistoryLoadMoreDispatchesGroupPathInsideChatFeature)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;

    int groupLoadingProjectionCount = 0;
    int privateLoadingProjectionCount = 0;
    bool groupLoadingProjected = false;
    bool privateLoadingProjected = false;
    GroupHistoryRequestPort groupPort;
    groupPort.setGroupHistoryLoading = [&groupLoadingProjectionCount, &groupLoadingProjected](bool loading)
    {
        ++groupLoadingProjectionCount;
        groupLoadingProjected = loading;
    };
    groupPort.setPrivateHistoryLoading = [&privateLoadingProjectionCount, &privateLoadingProjected](bool loading)
    {
        ++privateLoadingProjectionCount;
        privateLoadingProjected = loading;
    };

    ChatHistoryLoadMoreRequest request;
    request.selfUid = 1001;
    request.currentGroupId = 7001;
    request.privateHistoryLoading = false;
    request.canLoadMorePrivateHistory = true;
    request.groupHistoryLoading = false;
    request.groupLimit = 35;

    const ChatHistoryLoadMoreResult result = controller.loadMoreCurrentHistory(request,
                                                                               PrivateHistoryLoadMoreDependencies{},
                                                                               harness.dependencies(),
                                                                               groupPort);

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.groupPath);
    EXPECT_FALSE(result.privatePath);
    EXPECT_TRUE(result.dispatched);
    EXPECT_TRUE(result.groupLoadingProjected);
    EXPECT_EQ(harness.historyDispatchCount, 1);
    EXPECT_EQ(groupLoadingProjectionCount, 1);
    EXPECT_TRUE(groupLoadingProjected);
    EXPECT_EQ(privateLoadingProjectionCount, 1);
    EXPECT_TRUE(privateLoadingProjected);

    const QJsonObject payload = compactJsonObject(harness.lastHistoryPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("groupid")).toVariant().toLongLong(), 7001);
    EXPECT_EQ(payload.value(QStringLiteral("limit")).toInt(), 35);
}

TEST(ChatFeatureControllerTest, CurrentHistoryLoadMoreDispatchesPrivatePathInsideChatFeature)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    ChatMessageModel messageModel;
    auto oldMessage = std::make_shared<TextChatData>(QStringLiteral("p-old"),
        QStringLiteral("old private"),
                       2002,
                       1001,
                       QStringLiteral("Alice"), 1700000001000LL, QStringLiteral("a.png"), QStringLiteral("sent"));
    messageModel.setMessages({oldMessage}, 1001);

    int loadingProjectionCount = 0;
    bool loadingProjected = false;
    int dispatchCount = 0;
    int dispatchedReqId = 0;
    QByteArray dispatchedPayload;
    PrivateHistoryLoadMoreDependencies dependencies;
    dependencies.messageModel = &messageModel;
    dependencies.setLoading = [&loadingProjectionCount, &loadingProjected](bool loading)
    {
        ++loadingProjectionCount;
        loadingProjected = loading;
    };
    dependencies.dispatchPayload =
        [&dispatchCount, &dispatchedReqId, &dispatchedPayload](int reqId, const QByteArray& payload)
    {
        ++dispatchCount;
        dispatchedReqId = reqId;
        dispatchedPayload = payload;
    };

    ChatHistoryLoadMoreRequest request;
    request.selfUid = 1001;
    request.currentPrivatePeerUid = 2002;
    request.privateHistoryLoading = false;
    request.canLoadMorePrivateHistory = true;

    const ChatHistoryLoadMoreResult result =
        controller.loadMoreCurrentHistory(request, dependencies, GroupConversationDependencies{});

    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.groupPath);
    EXPECT_TRUE(result.privatePath);
    EXPECT_TRUE(result.dispatched);
    EXPECT_TRUE(result.loadingSet);
    EXPECT_EQ(result.peerUid, 2002);
    EXPECT_EQ(result.beforeTs, 1700000001000LL);
    EXPECT_EQ(result.beforeMsgId, QStringLiteral("p-old"));
    EXPECT_EQ(dispatchCount, 1);
    EXPECT_EQ(dispatchedReqId, 1059);
    EXPECT_EQ(loadingProjectionCount, 1);
    EXPECT_TRUE(loadingProjected);
    EXPECT_EQ(controller.privateHistoryState().pendingPeerUid, 2002);

    const QJsonObject payload = compactJsonObject(dispatchedPayload);
    EXPECT_EQ(payload.value(QStringLiteral("fromuid")).toInt(), 1001);
    EXPECT_EQ(payload.value(QStringLiteral("peer_uid")).toInt(), 2002);
    EXPECT_EQ(payload.value(QStringLiteral("before_ts")).toVariant().toLongLong(), 1700000001000LL);
    EXPECT_EQ(payload.value(QStringLiteral("before_msg_id")).toString(), QStringLiteral("p-old"));
}

TEST(ChatFeatureControllerTest, CurrentHistoryLoadMoreSkipsBeforeChoosingPathWhenUnavailable)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;

    int groupLoadingProjectionCount = 0;
    GroupHistoryRequestPort groupPort;
    groupPort.setGroupHistoryLoading = [&groupLoadingProjectionCount](bool)
    {
        ++groupLoadingProjectionCount;
    };

    ChatHistoryLoadMoreRequest loadingRequest;
    loadingRequest.selfUid = 1001;
    loadingRequest.currentGroupId = 7001;
    loadingRequest.privateHistoryLoading = true;
    loadingRequest.canLoadMorePrivateHistory = true;
    const ChatHistoryLoadMoreResult loadingResult =
        controller.loadMoreCurrentHistory(loadingRequest,
                                          PrivateHistoryLoadMoreDependencies{},
                                          harness.dependencies(),
                                          groupPort);

    EXPECT_TRUE(loadingResult.skipped);
    EXPECT_FALSE(loadingResult.groupPath);
    EXPECT_FALSE(loadingResult.privatePath);
    EXPECT_EQ(harness.historyDispatchCount, 0);
    EXPECT_EQ(groupLoadingProjectionCount, 0);

    ChatHistoryLoadMoreRequest noMoreRequest = loadingRequest;
    noMoreRequest.privateHistoryLoading = false;
    noMoreRequest.canLoadMorePrivateHistory = false;
    const ChatHistoryLoadMoreResult noMoreResult =
        controller.loadMoreCurrentHistory(noMoreRequest,
                                          PrivateHistoryLoadMoreDependencies{},
                                          harness.dependencies(),
                                          groupPort);

    EXPECT_TRUE(noMoreResult.skipped);
    EXPECT_FALSE(noMoreResult.groupPath);
    EXPECT_FALSE(noMoreResult.privatePath);
    EXPECT_EQ(harness.historyDispatchCount, 0);
    EXPECT_EQ(groupLoadingProjectionCount, 0);
}

TEST(ChatFeatureControllerTest, GroupOpenConversationOwnsModelAndReadAckDecision)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;
    auto group = harness.groups.value(7001);
    group->_chat_msgs.push_back(std::make_shared<TextChatData>(
        QStringLiteral("open-1"), QStringLiteral("first"), 2002, 0, QStringLiteral("Alice"), 1700000004000LL));
    group->_chat_msgs.push_back(std::make_shared<TextChatData>(
        QStringLiteral("open-2"), QStringLiteral("latest"), 2003, 0, QStringLiteral("Bob"), 1700000005000LL));
    controller.groupConversationState().historyBeforeSeq = 99;
    controller.groupConversationState().historyHasMore = false;

    GroupOpenRequest request;
    request.context = harness.context();
    request.groupId = 7001;
    const GroupOpenResult result = controller.openGroupConversation(request, harness.dependencies());

    ASSERT_TRUE(result.success);
    EXPECT_TRUE(result.modelSet);
    EXPECT_EQ(harness.messageModel.rowCount(), 2);
    EXPECT_EQ(modelValue(harness.messageModel, 1, ChatMessageModel::RawContentRole).toString(),
              QStringLiteral("latest"));
    EXPECT_EQ(result.requestedReadAckTs, 1700000005000LL);
    EXPECT_EQ(controller.groupConversationState().historyBeforeSeq, 0);
    EXPECT_TRUE(controller.groupConversationState().historyHasMore);
}

TEST(ChatFeatureControllerTest, GroupIncomingMessageUpdatesCurrentOrBackgroundConversation)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;

    QJsonObject currentMsg;
    currentMsg.insert(QStringLiteral("msgid"), QStringLiteral("in-1"));
    currentMsg.insert(QStringLiteral("content"), QStringLiteral("current hello"));
    currentMsg.insert(QStringLiteral("created_at"), static_cast<qint64>(1700000002000LL));
    GroupIncomingRequest currentRequest;
    currentRequest.context = harness.context(7001, 0);
    currentRequest.message =
        std::make_shared<GroupChatMsg>(7001, 2002, currentMsg, QStringLiteral("Alice"), QStringLiteral("a.png"));

    const GroupIncomingResult currentResult =
        controller.handleGroupIncomingMessage(currentRequest, harness.dependencies());

    ASSERT_TRUE(currentResult.success);
    EXPECT_TRUE(currentResult.modelAppended);
    EXPECT_TRUE(currentResult.unreadCleared);
    EXPECT_EQ(currentResult.requestedReadAckTs, 1700000002000LL);
    EXPECT_EQ(harness.messageModel.rowCount(), 1);
    EXPECT_EQ(modelValue(harness.messageModel, 0, ChatMessageModel::RawContentRole).toString(),
              QStringLiteral("current hello"));

    QJsonObject backgroundMsg;
    backgroundMsg.insert(QStringLiteral("msgid"), QStringLiteral("in-2"));
    backgroundMsg.insert(QStringLiteral("content"), QStringLiteral("background @all"));
    backgroundMsg.insert(QStringLiteral("mention_all"), true);
    backgroundMsg.insert(QStringLiteral("created_at"), static_cast<qint64>(1700000003000LL));
    GroupIncomingRequest backgroundRequest;
    backgroundRequest.context = harness.context(7001, 2002);
    backgroundRequest.message =
        std::make_shared<GroupChatMsg>(8001, 2003, backgroundMsg, QStringLiteral("Bob"), QStringLiteral("b.png"));

    const GroupIncomingResult backgroundResult =
        controller.handleGroupIncomingMessage(backgroundRequest, harness.dependencies());

    ASSERT_TRUE(backgroundResult.success);
    EXPECT_TRUE(backgroundResult.ensuredGroup);
    EXPECT_TRUE(backgroundResult.unreadIncremented);
    EXPECT_TRUE(backgroundResult.mentionedMe);
    EXPECT_FALSE(backgroundResult.modelAppended);
    EXPECT_EQ(harness.dialogListModel.indexOfUid(backgroundResult.dialogUid), 1);
    EXPECT_EQ(listValue(harness.dialogListModel, backgroundResult.dialogUid, QStringLiteral("unreadCount")).toInt(), 1);
    EXPECT_EQ(listValue(harness.dialogListModel, backgroundResult.dialogUid, QStringLiteral("mentionCount")).toInt(),
                        1);
    EXPECT_EQ(listValue(harness.dialogListModel, backgroundResult.dialogUid, QStringLiteral("lastMsg")).toString(),
                        QStringLiteral("[有人@你] background @all"));
}

TEST(ChatFeatureControllerTest, GroupConversationUsesFeatureOwnedDialogDefaults)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;
    const int dialogUid = harness.dialogUidFor(7001);
    controller.dialogListModel().upsertFriend(
        makeListDialog(dialogUid, QStringLiteral("Team"), QStringLiteral("group")));

    auto deps = harness.dependencies();
    deps.dialogListModel = nullptr;
    deps.clearGroupUnreadAndMention = {};
    deps.incrementGroupUnreadAndMention = {};
    deps.dialogDecorationState = {};

    GroupSendRequest sendRequest;
    sendRequest.context = harness.context();
    sendRequest.groupId = 7001;
    sendRequest.content = QStringLiteral("feature default send");

    const GroupSendResult sendResult = controller.sendGroupText(sendRequest, deps);

    ASSERT_TRUE(sendResult.success);
    EXPECT_TRUE(sendResult.previewUpdated);
    EXPECT_TRUE(sendResult.unreadCleared);
    EXPECT_EQ(listValue(controller.dialogListModel(), dialogUid, QStringLiteral("lastMsg")).toString(),
                        QStringLiteral("feature default send"));
    EXPECT_EQ(listValue(controller.dialogListModel(), dialogUid, QStringLiteral("unreadCount")).toInt(), 0);

    QJsonObject backgroundMsg;
    backgroundMsg.insert(QStringLiteral("msgid"), QStringLiteral("group-default-in"));
    backgroundMsg.insert(QStringLiteral("content"), QStringLiteral("background @all"));
    backgroundMsg.insert(QStringLiteral("mention_all"), true);
    backgroundMsg.insert(QStringLiteral("created_at"), static_cast<qint64>(1700000006000LL));
    GroupIncomingRequest backgroundRequest;
    backgroundRequest.context = harness.context(0, 2002);
    backgroundRequest.message =
        std::make_shared<GroupChatMsg>(7001, 2003, backgroundMsg, QStringLiteral("Bob"), QStringLiteral("b.png"));

    const GroupIncomingResult incomingResult = controller.handleGroupIncomingMessage(backgroundRequest, deps);

    ASSERT_TRUE(incomingResult.success);
    EXPECT_TRUE(incomingResult.unreadIncremented);
    EXPECT_TRUE(incomingResult.mentionedMe);
    EXPECT_EQ(listValue(controller.dialogListModel(), dialogUid, QStringLiteral("lastMsg")).toString(),
                        QStringLiteral("[有人@你] background @all"));
    EXPECT_EQ(listValue(controller.dialogListModel(), dialogUid, QStringLiteral("unreadCount")).toInt(), 1);
    EXPECT_EQ(listValue(controller.dialogListModel(), dialogUid, QStringLiteral("mentionCount")).toInt(), 1);
    EXPECT_EQ(controller.dialogDecorationState().mentionMap->value(dialogUid, 0), 1);
}

TEST(ChatFeatureControllerTest, GroupAckStatusAndErrorPatchPendingMessages)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;

    GroupSendRequest sendRequest;
    sendRequest.context = harness.context();
    sendRequest.groupId = 7001;
    sendRequest.content = QStringLiteral("pending");
    const GroupSendResult sendResult = controller.sendGroupText(sendRequest, harness.dependencies());
    ASSERT_TRUE(sendResult.success);

    QJsonObject msgObj;
    msgObj.insert(QStringLiteral("msgid"), sendResult.msgId);
    msgObj.insert(QStringLiteral("content"), QStringLiteral("persisted"));
    msgObj.insert(QStringLiteral("created_at"), static_cast<qint64>(4000));
    msgObj.insert(QStringLiteral("server_msg_id"), static_cast<qint64>(44));
    msgObj.insert(QStringLiteral("group_seq"), static_cast<qint64>(45));
    QJsonObject statusPayload;
    statusPayload.insert(QStringLiteral("client_msg_id"), sendResult.msgId);
    statusPayload.insert(QStringLiteral("error"), 0);
    statusPayload.insert(QStringLiteral("status"), QStringLiteral("persisted"));
    statusPayload.insert(QStringLiteral("msg"), msgObj);

    GroupAckRequest statusRequest;
    statusRequest.context = harness.context();
    statusRequest.payload = statusPayload;
    const GroupAckResult statusResult = controller.handleGroupMessageStatus(statusRequest, harness.dependencies());

    ASSERT_TRUE(statusResult.success);
    EXPECT_TRUE(statusResult.correctedMessage);
    EXPECT_TRUE(statusResult.pendingRemoved);
    EXPECT_EQ(statusResult.groupId, 7001);
    EXPECT_FALSE(controller.groupConversationState().pendingMsgGroupMap.contains(sendResult.msgId));
    EXPECT_EQ(modelValue(harness.messageModel, 0, ChatMessageModel::RawContentRole).toString(),
              QStringLiteral("persisted"));
    EXPECT_EQ(modelValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("sent"));

    GroupSendRequest failedSendRequest;
    failedSendRequest.context = harness.context();
    failedSendRequest.groupId = 7001;
    failedSendRequest.content = QStringLiteral("will fail");
    const GroupSendResult failedSend = controller.sendGroupText(failedSendRequest, harness.dependencies());
    ASSERT_TRUE(failedSend.success);

    QJsonObject errorPayload;
    errorPayload.insert(QStringLiteral("client_msg_id"), failedSend.msgId);
    GroupAckRequest errorRequest;
    errorRequest.context = harness.context();
    errorRequest.errorCode = 409;
    errorRequest.payload = errorPayload;
    const GroupAckResult errorResult = controller.handleGroupMessageError(errorRequest, harness.dependencies());

    ASSERT_TRUE(errorResult.success);
    EXPECT_EQ(errorResult.statusText, QStringLiteral("发送群消息失败（错误码:409）"));
    EXPECT_TRUE(errorResult.statusIsError);
    EXPECT_EQ(errorResult.groupId, 7001);
    EXPECT_TRUE(errorResult.pendingRemoved);
    EXPECT_FALSE(controller.groupConversationState().pendingMsgGroupMap.contains(failedSend.msgId));
    EXPECT_EQ(modelValue(harness.messageModel, 1, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("failed"));
}

TEST(ChatFeatureControllerTest, GroupMutationAndReadAckPatchMessages)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;
    auto group = harness.groups.value(7001);
    group->_chat_msgs.push_back(std::make_shared<TextChatData>(
        QStringLiteral("m-1"), QStringLiteral("before"), 1001, 0, QStringLiteral("Me"), 5000));
    harness.messageModel.setMessages(group->_chat_msgs, 1001);

    QJsonObject editPayload;
    editPayload.insert(QStringLiteral("groupid"), static_cast<qint64>(7001));
    editPayload.insert(QStringLiteral("msgid"), QStringLiteral("m-1"));
    editPayload.insert(QStringLiteral("content"), QStringLiteral("after"));
    editPayload.insert(QStringLiteral("edited_at_ms"), static_cast<qint64>(6000));
    GroupMutationRequest editRequest;
    editRequest.context = harness.context();
    editRequest.payload = editPayload;
    editRequest.event = QStringLiteral("group_msg_edited");
    const GroupMutationResult editResult = controller.handleGroupMessageMutation(editRequest, harness.dependencies());

    ASSERT_TRUE(editResult.success);
    EXPECT_EQ(editResult.statusText, QStringLiteral("群消息已编辑"));
    EXPECT_FALSE(editResult.statusIsError);
    EXPECT_TRUE(editResult.modelSet);
    EXPECT_EQ(modelValue(harness.messageModel, 0, ChatMessageModel::RawContentRole).toString(),
              QStringLiteral("after"));
    EXPECT_EQ(modelValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("edited"));

    QJsonObject revokePayload;
    revokePayload.insert(QStringLiteral("groupid"), static_cast<qint64>(7001));
    revokePayload.insert(QStringLiteral("msgid"), QStringLiteral("m-1"));
    revokePayload.insert(QStringLiteral("deleted_at_ms"), static_cast<qint64>(7000));
    GroupMutationRequest revokeRequest;
    revokeRequest.context = harness.context();
    revokeRequest.reqId = 1068;
    revokeRequest.payload = revokePayload;
    const GroupMutationResult revokeResult =
        controller.handleGroupMessageMutation(revokeRequest, harness.dependencies());

    ASSERT_TRUE(revokeResult.success);
    EXPECT_EQ(revokeResult.statusText, QStringLiteral("群消息已撤回"));
    EXPECT_FALSE(revokeResult.statusIsError);
    EXPECT_EQ(modelValue(harness.messageModel, 0, ChatMessageModel::RawContentRole).toString(),
              QStringLiteral("[消息已撤回]"));
    EXPECT_EQ(modelValue(harness.messageModel, 0, ChatMessageModel::MessageStateRole).toString(),
              QStringLiteral("deleted"));

    QJsonObject readAckPayload;
    readAckPayload.insert(QStringLiteral("groupid"), static_cast<qint64>(7001));
    readAckPayload.insert(QStringLiteral("fromuid"), 2002);
    readAckPayload.insert(QStringLiteral("read_ts"), static_cast<qint64>(8));
    GroupReadAckRequest readAckRequest;
    readAckRequest.context = harness.context();
    readAckRequest.payload = readAckPayload;
    const GroupReadAckResult readAckResult = controller.handleGroupReadAckEvent(readAckRequest, harness.dependencies());

    ASSERT_TRUE(readAckResult.success);
    EXPECT_EQ(readAckResult.updatedCount, 1);
    EXPECT_EQ(harness.outgoingReadGroupId, 7001);
    EXPECT_EQ(harness.outgoingReadOwnerUid, 1001);
    EXPECT_EQ(harness.outgoingReadTs, 8000);
    EXPECT_TRUE(readAckResult.modelSet);
}

TEST(ChatFeatureControllerTest, GroupForwardAckOwnsStatusAndReadAckPolicy)
{
    ChatViewModel viewModel;
    ChatFeatureController controller(viewModel);
    GroupConversationHarness harness;

    GroupSendRequest sendRequest;
    sendRequest.context = harness.context();
    sendRequest.groupId = 7001;
    sendRequest.content = QStringLiteral("forward pending");
    const GroupSendResult sendResult = controller.sendGroupText(sendRequest, harness.dependencies());
    ASSERT_TRUE(sendResult.success);

    QJsonObject msgObj;
    msgObj.insert(QStringLiteral("msgid"), sendResult.msgId);
    msgObj.insert(QStringLiteral("content"), QStringLiteral("forward persisted"));
    msgObj.insert(QStringLiteral("created_at"), static_cast<qint64>(1700000009000LL));
    msgObj.insert(QStringLiteral("server_msg_id"), static_cast<qint64>(55));
    msgObj.insert(QStringLiteral("group_seq"), static_cast<qint64>(56));

    QJsonObject ackPayload;
    ackPayload.insert(QStringLiteral("groupid"), static_cast<qint64>(7001));
    ackPayload.insert(QStringLiteral("client_msg_id"), sendResult.msgId);
    ackPayload.insert(QStringLiteral("msg"), msgObj);

    GroupAckRequest ackRequest;
    ackRequest.context = harness.context();
    ackRequest.reqId = 1070;
    ackRequest.payload = ackPayload;
    const GroupAckResult ackResult = controller.handleGroupMessageAck(ackRequest, harness.dependencies());

    ASSERT_TRUE(ackResult.success);
    EXPECT_EQ(ackResult.statusText, QStringLiteral("消息已转发"));
    EXPECT_FALSE(ackResult.statusIsError);
    EXPECT_EQ(ackResult.requestedReadAckTs, 1700000009000LL);
    EXPECT_EQ(ackResult.groupId, 7001);
}

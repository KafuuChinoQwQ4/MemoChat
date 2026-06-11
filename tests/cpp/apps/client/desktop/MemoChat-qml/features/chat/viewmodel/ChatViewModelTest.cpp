#include "ChatViewModel.h"

#include <gtest/gtest.h>

#include <QVariantMap>

#include <cstdint>

TEST(ChatViewModelTest, SyncStateEmitsOnlyWhenObservedValuesChange)
{
    ChatViewModel viewModel;
    int stateChangedCount = 0;
    QObject::connect(&viewModel,
                     &ChatViewModel::stateChanged,
                     [&stateChangedCount]()
                     {
                         ++stateChangedCount;
                     });

    EXPECT_EQ(viewModel.chatTab(), 0);
    viewModel.syncChatTab(1);
    EXPECT_EQ(viewModel.chatTab(), 1);
    EXPECT_EQ(stateChangedCount, 1);
    viewModel.syncChatTab(1);
    EXPECT_EQ(stateChangedCount, 1);

    viewModel.syncCurrentPeer(QStringLiteral("Alice"), QStringLiteral("qrc:/res/alice.png"), true);
    EXPECT_EQ(viewModel.currentChatPeerName(), QStringLiteral("Alice"));
    EXPECT_EQ(viewModel.currentChatPeerIcon(), QStringLiteral("qrc:/res/alice.png"));
    EXPECT_TRUE(viewModel.hasCurrentChat());
    EXPECT_EQ(stateChangedCount, 2);
    viewModel.syncCurrentPeer(QStringLiteral("Alice"), QStringLiteral("qrc:/res/alice.png"), true);
    EXPECT_EQ(stateChangedCount, 2);

    QVariantMap imageAttachment;
    imageAttachment.insert(QStringLiteral("id"), QStringLiteral("att-1"));
    imageAttachment.insert(QStringLiteral("type"), QStringLiteral("image"));
    const QVariantList attachments{imageAttachment};

    viewModel.syncCurrentPendingAttachments(attachments);
    EXPECT_TRUE(viewModel.hasPendingAttachments());
    EXPECT_EQ(viewModel.currentPendingAttachments(), attachments);
    EXPECT_EQ(stateChangedCount, 3);
    viewModel.syncCurrentPendingAttachments(attachments);
    EXPECT_EQ(stateChangedCount, 3);

    viewModel.syncMediaUpload(true, QStringLiteral("45%"));
    EXPECT_TRUE(viewModel.mediaUploadInProgress());
    EXPECT_EQ(viewModel.mediaUploadProgressText(), QStringLiteral("45%"));
    EXPECT_EQ(stateChangedCount, 4);
    viewModel.syncMediaUpload(true, QStringLiteral("45%"));
    EXPECT_EQ(stateChangedCount, 4);
}

TEST(ChatViewModelTest, SyncModelsEmitOnlyWhenPointersChange)
{
    ChatViewModel viewModel;
    int modelChangedCount = 0;
    QObject::connect(&viewModel,
                     &ChatViewModel::modelChanged,
                     [&modelChangedCount]()
                     {
                         ++modelChangedCount;
                     });

    auto* const dialogModel = reinterpret_cast<FriendListModel*>(static_cast<std::uintptr_t>(0x1));
    auto* const messageModel = reinterpret_cast<ChatMessageModel*>(static_cast<std::uintptr_t>(0x2));

    viewModel.syncDialogListModel(dialogModel);
    EXPECT_EQ(viewModel.dialogListModel(), dialogModel);
    EXPECT_EQ(modelChangedCount, 1);
    viewModel.syncDialogListModel(dialogModel);
    EXPECT_EQ(modelChangedCount, 1);

    viewModel.syncMessageModel(messageModel);
    EXPECT_EQ(viewModel.messageModel(), messageModel);
    EXPECT_EQ(modelChangedCount, 2);
    viewModel.syncMessageModel(messageModel);
    EXPECT_EQ(modelChangedCount, 2);
}

TEST(ChatViewModelTest, ReplyAndDialogFlagsAreSynchronizedAsSingleStateChanges)
{
    ChatViewModel viewModel;
    int stateChangedCount = 0;
    QObject::connect(&viewModel,
                     &ChatViewModel::stateChanged,
                     [&stateChangedCount]()
                     {
                         ++stateChangedCount;
                     });

    viewModel.syncCurrentDialogPinned(true);
    viewModel.syncCurrentDialogMuted(true);
    viewModel.syncPendingReply(true, QStringLiteral("Alice"), QStringLiteral("hello"));

    EXPECT_TRUE(viewModel.currentDialogPinned());
    EXPECT_TRUE(viewModel.currentDialogMuted());
    EXPECT_TRUE(viewModel.hasPendingReply());
    EXPECT_EQ(viewModel.replyTargetName(), QStringLiteral("Alice"));
    EXPECT_EQ(viewModel.replyPreviewText(), QStringLiteral("hello"));
    EXPECT_EQ(stateChangedCount, 3);

    viewModel.syncCurrentDialogPinned(true);
    viewModel.syncCurrentDialogMuted(true);
    viewModel.syncPendingReply(true, QStringLiteral("Alice"), QStringLiteral("hello"));
    EXPECT_EQ(stateChangedCount, 3);

    viewModel.syncPendingReply(false, QString(), QString());
    EXPECT_FALSE(viewModel.hasPendingReply());
    EXPECT_TRUE(viewModel.replyTargetName().isEmpty());
    EXPECT_TRUE(viewModel.replyPreviewText().isEmpty());
    EXPECT_EQ(stateChangedCount, 4);
}

TEST(ChatViewModelTest, InvokableCommandsEmitRequestSignalsWithArguments)
{
    ChatViewModel viewModel;

    int ensureChatListCount = 0;
    int loadMorePrivateHistoryCount = 0;
    int ensureGroupsCount = 0;
    int loadMoreChatsCount = 0;
    int sendImageCount = 0;
    int sendFileCount = 0;
    int clearAttachmentsCount = 0;
    int cancelReplyCount = 0;
    int toggleCurrentPinnedCount = 0;
    int toggleCurrentMutedCount = 0;
    int startVoiceCount = 0;
    int startVideoCount = 0;
    int selectedDialogUid = 0;
    int selectedChatIndex = 0;
    int selectedGroupIndex = 0;
    int toggledPinnedUid = 0;
    int toggledMutedUid = 0;
    int markedReadUid = 0;
    int clearedDraftUid = 0;
    QString sentText;
    QString removedAttachmentId;
    QString draftText;
    QString replyMsgId;
    QString replySenderName;
    QString replyPreviewText;

    QObject::connect(&viewModel,
                     &ChatViewModel::ensureChatListInitializedRequested,
                     [&ensureChatListCount]()
                     {
                         ++ensureChatListCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::loadMorePrivateHistoryRequested,
                     [&loadMorePrivateHistoryCount]()
                     {
                         ++loadMorePrivateHistoryCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::ensureGroupsInitializedRequested,
                     [&ensureGroupsCount]()
                     {
                         ++ensureGroupsCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::loadMoreChatsRequested,
                     [&loadMoreChatsCount]()
                     {
                         ++loadMoreChatsCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::sendImageMessageRequested,
                     [&sendImageCount]()
                     {
                         ++sendImageCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::sendFileMessageRequested,
                     [&sendFileCount]()
                     {
                         ++sendFileCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::clearPendingAttachmentsRequested,
                     [&clearAttachmentsCount]()
                     {
                         ++clearAttachmentsCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::cancelReplyMessageRequested,
                     [&cancelReplyCount]()
                     {
                         ++cancelReplyCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::toggleCurrentDialogPinnedRequested,
                     [&toggleCurrentPinnedCount]()
                     {
                         ++toggleCurrentPinnedCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::toggleCurrentDialogMutedRequested,
                     [&toggleCurrentMutedCount]()
                     {
                         ++toggleCurrentMutedCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::startVoiceChatRequested,
                     [&startVoiceCount]()
                     {
                         ++startVoiceCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::startVideoChatRequested,
                     [&startVideoCount]()
                     {
                         ++startVideoCount;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::selectDialogByUidRequested,
                     [&selectedDialogUid](int uid)
                     {
                         selectedDialogUid = uid;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::selectChatIndexRequested,
                     [&selectedChatIndex](int index)
                     {
                         selectedChatIndex = index;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::selectGroupIndexRequested,
                     [&selectedGroupIndex](int index)
                     {
                         selectedGroupIndex = index;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::toggleDialogPinnedByUidRequested,
                     [&toggledPinnedUid](int uid)
                     {
                         toggledPinnedUid = uid;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::toggleDialogMutedByUidRequested,
                     [&toggledMutedUid](int uid)
                     {
                         toggledMutedUid = uid;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::markDialogReadByUidRequested,
                     [&markedReadUid](int uid)
                     {
                         markedReadUid = uid;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::clearDialogDraftByUidRequested,
                     [&clearedDraftUid](int uid)
                     {
                         clearedDraftUid = uid;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::sendCurrentComposerPayloadRequested,
                     [&sentText](const QString& text)
                     {
                         sentText = text;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::removePendingAttachmentRequested,
                     [&removedAttachmentId](const QString& attachmentId)
                     {
                         removedAttachmentId = attachmentId;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::updateCurrentDraftRequested,
                     [&draftText](const QString& text)
                     {
                         draftText = text;
                     });
    QObject::connect(&viewModel,
                     &ChatViewModel::beginReplyMessageRequested,
                     [&replyMsgId, &replySenderName, &replyPreviewText](const QString& msgId,
                                                                        const QString& senderName,
                                                                        const QString& previewText)
                     {
                         replyMsgId = msgId;
                         replySenderName = senderName;
                         replyPreviewText = previewText;
                     });

    viewModel.ensureChatListInitialized();
    viewModel.loadMorePrivateHistory();
    viewModel.ensureGroupsInitialized();
    viewModel.loadMoreChats();
    viewModel.sendImageMessage();
    viewModel.sendFileMessage();
    viewModel.clearPendingAttachments();
    viewModel.cancelReplyMessage();
    viewModel.toggleCurrentDialogPinned();
    viewModel.toggleCurrentDialogMuted();
    viewModel.startVoiceChat();
    viewModel.startVideoChat();
    viewModel.selectDialogByUid(1060);
    viewModel.selectChatIndex(2);
    viewModel.selectGroupIndex(3);
    viewModel.toggleDialogPinnedByUid(-88);
    viewModel.toggleDialogMutedByUid(-89);
    viewModel.markDialogReadByUid(1061);
    viewModel.clearDialogDraftByUid(1062);
    viewModel.sendCurrentComposerPayload(QStringLiteral("hello"));
    viewModel.removePendingAttachment(QStringLiteral("att-1"));
    viewModel.updateCurrentDraft(QStringLiteral("draft"));
    viewModel.beginReplyMessage(QStringLiteral("m1"), QStringLiteral("Alice"), QStringLiteral("preview"));

    EXPECT_EQ(ensureChatListCount, 1);
    EXPECT_EQ(loadMorePrivateHistoryCount, 1);
    EXPECT_EQ(ensureGroupsCount, 1);
    EXPECT_EQ(loadMoreChatsCount, 1);
    EXPECT_EQ(sendImageCount, 1);
    EXPECT_EQ(sendFileCount, 1);
    EXPECT_EQ(clearAttachmentsCount, 1);
    EXPECT_EQ(cancelReplyCount, 1);
    EXPECT_EQ(toggleCurrentPinnedCount, 1);
    EXPECT_EQ(toggleCurrentMutedCount, 1);
    EXPECT_EQ(startVoiceCount, 1);
    EXPECT_EQ(startVideoCount, 1);
    EXPECT_EQ(selectedDialogUid, 1060);
    EXPECT_EQ(selectedChatIndex, 2);
    EXPECT_EQ(selectedGroupIndex, 3);
    EXPECT_EQ(toggledPinnedUid, -88);
    EXPECT_EQ(toggledMutedUid, -89);
    EXPECT_EQ(markedReadUid, 1061);
    EXPECT_EQ(clearedDraftUid, 1062);
    EXPECT_EQ(sentText, QStringLiteral("hello"));
    EXPECT_EQ(removedAttachmentId, QStringLiteral("att-1"));
    EXPECT_EQ(draftText, QStringLiteral("draft"));
    EXPECT_EQ(replyMsgId, QStringLiteral("m1"));
    EXPECT_EQ(replySenderName, QStringLiteral("Alice"));
    EXPECT_EQ(replyPreviewText, QStringLiteral("preview"));
}

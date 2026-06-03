#ifndef CHATVIEWMODEL_H
#define CHATVIEWMODEL_H

#include "ChatUiState.h"

#include <QObject>
#include <QString>
#include <QVariantList>

class ChatMessageModel;
class FriendListModel;

class ChatViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int chatTab READ chatTab NOTIFY stateChanged)
    Q_PROPERTY(int currentDialogUid READ currentDialogUid NOTIFY stateChanged)
    Q_PROPERTY(QString currentChatPeerName READ currentChatPeerName NOTIFY stateChanged)
    Q_PROPERTY(QString currentChatPeerIcon READ currentChatPeerIcon NOTIFY stateChanged)
    Q_PROPERTY(bool hasCurrentChat READ hasCurrentChat NOTIFY stateChanged)
    Q_PROPERTY(bool hasCurrentGroup READ hasCurrentGroup NOTIFY stateChanged)
    Q_PROPERTY(int currentGroupRole READ currentGroupRole NOTIFY stateChanged)
    Q_PROPERTY(FriendListModel* dialogListModel READ dialogListModel NOTIFY modelChanged)
    Q_PROPERTY(ChatMessageModel* messageModel READ messageModel NOTIFY modelChanged)
    Q_PROPERTY(QString currentDraftText READ currentDraftText NOTIFY stateChanged)
    Q_PROPERTY(QVariantList currentPendingAttachments READ currentPendingAttachments NOTIFY stateChanged)
    Q_PROPERTY(bool hasPendingAttachments READ hasPendingAttachments NOTIFY stateChanged)
    Q_PROPERTY(bool currentDialogPinned READ currentDialogPinned NOTIFY stateChanged)
    Q_PROPERTY(bool currentDialogMuted READ currentDialogMuted NOTIFY stateChanged)
    Q_PROPERTY(bool hasPendingReply READ hasPendingReply NOTIFY stateChanged)
    Q_PROPERTY(QString replyTargetName READ replyTargetName NOTIFY stateChanged)
    Q_PROPERTY(QString replyPreviewText READ replyPreviewText NOTIFY stateChanged)
    Q_PROPERTY(bool dialogsReady READ dialogsReady NOTIFY stateChanged)
    Q_PROPERTY(bool chatLoadingMore READ chatLoadingMore NOTIFY stateChanged)
    Q_PROPERTY(bool canLoadMoreChats READ canLoadMoreChats NOTIFY stateChanged)
    Q_PROPERTY(bool privateHistoryLoading READ privateHistoryLoading NOTIFY stateChanged)
    Q_PROPERTY(bool canLoadMorePrivateHistory READ canLoadMorePrivateHistory NOTIFY stateChanged)
    Q_PROPERTY(bool mediaUploadInProgress READ mediaUploadInProgress NOTIFY stateChanged)
    Q_PROPERTY(QString mediaUploadProgressText READ mediaUploadProgressText NOTIFY stateChanged)

public:
    explicit ChatViewModel(QObject* parent = nullptr);

    int chatTab() const;
    int currentDialogUid() const;
    QString currentChatPeerName() const;
    QString currentChatPeerIcon() const;
    bool hasCurrentChat() const;
    bool hasCurrentGroup() const;
    int currentGroupRole() const;
    FriendListModel* dialogListModel() const;
    ChatMessageModel* messageModel() const;
    QString currentDraftText() const;
    QVariantList currentPendingAttachments() const;
    bool hasPendingAttachments() const;
    bool currentDialogPinned() const;
    bool currentDialogMuted() const;
    bool hasPendingReply() const;
    QString replyTargetName() const;
    QString replyPreviewText() const;
    bool dialogsReady() const;
    bool chatLoadingMore() const;
    bool canLoadMoreChats() const;
    bool privateHistoryLoading() const;
    bool canLoadMorePrivateHistory() const;
    bool mediaUploadInProgress() const;
    QString mediaUploadProgressText() const;

    void syncDialogListModel(FriendListModel* model);
    void syncMessageModel(ChatMessageModel* model);
    void syncChatTab(int tab);
    void syncCurrentDialogUid(int uid);
    void syncCurrentPeer(const QString& name, const QString& icon, bool hasCurrentChat);
    void syncCurrentGroup(bool hasCurrentGroup, int role);
    void syncCurrentDraftText(const QString& text);
    void syncCurrentPendingAttachments(const QVariantList& attachments);
    void syncCurrentDialogPinned(bool pinned);
    void syncCurrentDialogMuted(bool muted);
    void syncPendingReply(bool hasReply, const QString& targetName, const QString& previewText);
    void syncDialogsReady(bool ready);
    void syncChatLoadingMore(bool loading);
    void syncCanLoadMoreChats(bool canLoad);
    void syncPrivateHistoryLoading(bool loading);
    void syncCanLoadMorePrivateHistory(bool canLoad);
    void syncMediaUpload(bool inProgress, const QString& progressText);

    Q_INVOKABLE void ensureChatListInitialized();
    Q_INVOKABLE void ensureGroupsInitialized();
    Q_INVOKABLE void selectDialogByUid(int uid);
    Q_INVOKABLE void selectChatIndex(int index);
    Q_INVOKABLE void selectGroupIndex(int index);
    Q_INVOKABLE void loadMoreChats();
    Q_INVOKABLE void loadMorePrivateHistory();
    Q_INVOKABLE void sendCurrentComposerPayload(const QString& text);
    Q_INVOKABLE void sendImageMessage();
    Q_INVOKABLE void sendFileMessage();
    Q_INVOKABLE void removePendingAttachment(const QString& attachmentId);
    Q_INVOKABLE void clearPendingAttachments();
    Q_INVOKABLE void updateCurrentDraft(const QString& text);
    Q_INVOKABLE void toggleCurrentDialogPinned();
    Q_INVOKABLE void toggleCurrentDialogMuted();
    Q_INVOKABLE void toggleDialogPinnedByUid(int dialogUid);
    Q_INVOKABLE void toggleDialogMutedByUid(int dialogUid);
    Q_INVOKABLE void markDialogReadByUid(int dialogUid);
    Q_INVOKABLE void clearDialogDraftByUid(int dialogUid);
    Q_INVOKABLE void beginReplyMessage(const QString& msgId, const QString& senderName, const QString& previewText);
    Q_INVOKABLE void cancelReplyMessage();
    Q_INVOKABLE void startVoiceChat();
    Q_INVOKABLE void startVideoChat();

signals:
    void stateChanged();
    void modelChanged();
    void ensureChatListInitializedRequested();
    void ensureGroupsInitializedRequested();
    void selectDialogByUidRequested(int uid);
    void selectChatIndexRequested(int index);
    void selectGroupIndexRequested(int index);
    void loadMoreChatsRequested();
    void loadMorePrivateHistoryRequested();
    void sendCurrentComposerPayloadRequested(QString text);
    void sendImageMessageRequested();
    void sendFileMessageRequested();
    void removePendingAttachmentRequested(QString attachmentId);
    void clearPendingAttachmentsRequested();
    void updateCurrentDraftRequested(QString text);
    void toggleCurrentDialogPinnedRequested();
    void toggleCurrentDialogMutedRequested();
    void toggleDialogPinnedByUidRequested(int dialogUid);
    void toggleDialogMutedByUidRequested(int dialogUid);
    void markDialogReadByUidRequested(int dialogUid);
    void clearDialogDraftByUidRequested(int dialogUid);
    void beginReplyMessageRequested(QString msgId, QString senderName, QString previewText);
    void cancelReplyMessageRequested();
    void startVoiceChatRequested();
    void startVideoChatRequested();

private:
    ChatUiState _state;
    FriendListModel* _dialogListModel = nullptr;
    ChatMessageModel* _messageModel = nullptr;
};

#endif // CHATVIEWMODEL_H

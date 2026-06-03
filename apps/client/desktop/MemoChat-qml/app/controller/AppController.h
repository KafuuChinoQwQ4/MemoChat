#ifndef APPCONTROLLER_H
#define APPCONTROLLER_H

#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <memory>
#include <QVariantList>
#include <QMap>
#include <QHash>
#include <QSet>
#include "global.h"
#include "AuthController.h"
#include "AuthService.h"
#include "AuthViewModel.h"
#include "AppControllerConnectionState.h"
#include "AppControllerDialogStateData.h"
#include "AppControllerGroupState.h"
#include "AppControllerPendingSendState.h"
#include "AppControllerRuntimeState.h"
#include "AppControllerUserState.h"
#include "CallController.h"
#include "CallSessionModel.h"
#include "ChatController.h"
#include "ChatViewModel.h"
#include "ClientGateway.h"
#include "ContactController.h"
#include "GroupController.h"
#include "FriendListModel.h"
#include "ChatMessageModel.h"
#include "SearchResultModel.h"
#include "ApplyRequestModel.h"
#include "ProfileController.h"
#include "MomentsController.h"
#include "MomentsModel.h"
#include "AgentController.h"
#include "AgentMessageModel.h"
#include "PetController.h"
#include "R18Controller.h"
#include "PrivateChatCacheStore.h"
#include "GroupChatCacheStore.h"
#include "LivekitBridge.h"
#include "MediaUploadService.h"
#include "ShellViewModel.h"

class AppSessionCoordinator;
class ContactCoordinatorShell;
class GroupCoordinator;
class MediaCoordinator;
class CallCoordinator;
class ProfileCoordinator;
class AppChatConnectionCoordinator;

class AppController : public QObject
{
    Q_OBJECT

public:
    enum Page
    {
        LoginPage = 0,
        RegisterPage = 1,
        ResetPage = 2,
        ChatPage = 3
    };
    Q_ENUM(Page)

    enum ChatTab
    {
        ChatTabPage = 0,
        ContactTabPage = 1,
        SettingsTabPage = 2,
        MomentsTabPage = 3,
        AgentTabPage = 4,
        Live2DTabPage = 5
    };
    Q_ENUM(ChatTab)

    enum ContactPane
    {
        ApplyRequestPane = 0,
        FriendInfoPane = 1
    };
    Q_ENUM(ContactPane)

    explicit AppController(QObject* parent = nullptr);
    ~AppController();

    Page page() const;
    QString tipText() const;
    bool tipError() const;
    bool busy() const;
    bool registerSuccessPage() const;
    int registerCountdown() const;
    int registerCodeCooldownSeconds() const;
    bool registerCodeRequestPending() const;
    ChatTab chatTab() const;
    ContactPane contactPane() const;
    QString currentUserName() const;
    QString currentUserNick() const;
    QString currentUserIcon() const;
    QString currentUserDesc() const;
    QString currentUserId() const;
    int currentUserUid() const;
    QString currentContactName() const;
    QString currentContactNick() const;
    QString currentContactIcon() const;
    QString currentContactBack() const;
    int currentContactSex() const;
    QString currentContactUserId() const;
    int currentContactUid() const;
    bool hasCurrentContact() const;
    QString currentChatPeerName() const;
    QString currentChatPeerIcon() const;
    bool hasCurrentChat() const;
    bool hasCurrentGroup() const;
    int currentGroupRole() const;
    QString currentGroupName() const;
    QString currentGroupCode() const;
    bool currentGroupCanChangeInfo() const;
    bool currentGroupCanDeleteMessages() const;
    bool currentGroupCanInviteUsers() const;
    bool currentGroupCanManageAdmins() const;
    bool currentGroupCanPinMessages() const;
    bool currentGroupCanBanUsers() const;
    bool currentGroupCanManageTopics() const;
    FriendListModel* dialogListModel();
    FriendListModel* chatListModel();
    FriendListModel* groupListModel();
    FriendListModel* contactListModel();
    ChatMessageModel* messageModel();
    SearchResultModel* searchResultModel();
    ApplyRequestModel* applyRequestModel();
    MomentsModel* momentsModel() const;
    MomentsController* momentsController() const;
    AgentController* agentController() const;
    AgentMessageModel* agentMessageModel() const;
    PetController* petController() const;
    R18Controller* r18Controller() const;
    ShellViewModel* shell();
    ShellViewModel* shellViewModel();
    AuthViewModel* auth();
    AuthViewModel* authViewModel();
    ChatViewModel* chat();
    ChatViewModel* chatViewModel();
    ContactController* contact();
    ContactController* contactController();
    GroupController* group();
    GroupController* groupController();
    ProfileController* profile();
    ProfileController* profileController();
    CallController* call();
    CallController* callController();
    bool searchPending() const;
    QString searchStatusText() const;
    bool searchStatusError() const;
    bool hasPendingApply() const;
    bool chatLoadingMore() const;
    bool canLoadMoreChats() const;
    bool privateHistoryLoading() const;
    bool canLoadMorePrivateHistory() const;
    bool contactLoadingMore() const;
    bool canLoadMoreContacts() const;
    QString authStatusText() const;
    bool authStatusError() const;
    QString settingsStatusText() const;
    bool settingsStatusError() const;
    QString groupStatusText() const;
    bool groupStatusError() const;
    bool mediaUploadInProgress() const;
    QString mediaUploadProgressText() const;
    CallSessionModel* callSession();
    LivekitBridge* livekitBridge();
    QString currentDraftText() const;
    QVariantList currentPendingAttachments() const;
    bool hasPendingAttachments() const;
    bool currentDialogPinned() const;
    bool currentDialogMuted() const;
    bool hasPendingReply() const;
    QString replyTargetName() const;
    QString replyPreviewText() const;
    bool dialogsReady() const;
    bool contactsReady() const;
    bool groupsReady() const;
    bool applyReady() const;
    bool chatShellBusy() const;

    void switchToLogin();
    void switchToRegister();
    void switchToReset();
    void switchChatTab(int tab);
    void ensureContactsInitialized();
    void ensureGroupsInitialized();
    void ensureApplyInitialized();
    void ensureChatListInitialized();
    void clearTip();
    void selectChatIndex(int index);
    void selectGroupIndex(int index);
    void selectDialogByUid(int uid);
    void selectContactIndex(int index);
    QVariantMap contactProfileByUid(int uid) const;
    void deleteFriend(int uid);
    void showApplyRequests();
    void jumpChatWithCurrentContact();
    void loadMoreChats();
    void loadMorePrivateHistory();
    void loadMoreContacts();
    void sendTextMessage(const QString& text);
    void sendCurrentComposerPayload(const QString& text);
    void sendImageMessage();
    void sendFileMessage();
    void removePendingAttachment(const QString& attachmentId);
    void clearPendingAttachments();
    void openExternalResource(const QString& url);
    void searchUser(const QString& uidText);
    void clearSearchState();
    void requestAddFriend(int uid, const QString& bakName, const QVariantList& labels = QVariantList());
    void approveFriend(int uid, const QString& backName, const QVariantList& labels = QVariantList());
    void clearAuthStatus();
    void startVoiceChat();
    void startVideoChat();
    void acceptIncomingCall();
    void rejectIncomingCall();
    void endCurrentCall();
    void toggleCallMuted();
    void toggleCallCamera();
    void chooseAvatar(int source = 0);
    void saveProfile(const QString& nick, const QString& desc);
    void clearSettingsStatus();
    void refreshGroupList();
    void createGroup(const QString& name, const QVariantList& memberUserIdList = QVariantList());
    void inviteGroupMember(const QString& userId, const QString& reason = QString());
    void applyJoinGroup(const QString& groupCode, const QString& reason = QString());
    void reviewGroupApply(qint64 applyId, bool agree);
    void sendGroupTextMessage(const QString& text);
    void sendGroupImageMessage();
    void sendGroupFileMessage();
    void editGroupMessage(const QString& msgId, const QString& text);
    void revokeGroupMessage(const QString& msgId);
    void forwardGroupMessage(const QString& msgId);
    void loadGroupHistory();
    void updateGroupAnnouncement(const QString& announcement);
    void updateGroupIcon(int source = 0);
    void setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits = 0);
    void muteGroupMember(const QString& userId, int muteSeconds);
    void kickGroupMember(const QString& userId);
    void quitCurrentGroup();
    void dissolveCurrentGroup();
    void clearGroupStatus();
    void updateCurrentDraft(const QString& text);
    void toggleCurrentDialogPinned();
    void toggleCurrentDialogMuted();
    void toggleDialogPinnedByUid(int dialogUid);
    void toggleDialogMutedByUid(int dialogUid);
    void markDialogReadByUid(int dialogUid);
    void clearDialogDraftByUid(int dialogUid);
    void beginReplyMessage(const QString& msgId, const QString& senderName, const QString& previewText);
    void cancelReplyMessage();

    QString loginCredentialCacheJson() const;
    void saveLoginCredential(const QString& email, const QString& password);

    void login(const QString& email, const QString& password);
    void beginPostLoginBootstrap();
    void requestRegisterCode(const QString& email);
    void registerUser(const QString& user,
                      const QString& email,
                      const QString& password,
                      const QString& confirm,
                      const QString& verifyCode);
    void requestResetCode(const QString& email);
    void resetPassword(const QString& user, const QString& email, const QString& password, const QString& verifyCode);

signals:
    void pageChanged();
    void tipChanged();
    void busyChanged();
    void registerSuccessPageChanged();
    void registerCountdownChanged();
    void registerCodeCooldownChanged();
    void loginCredentialCacheChanged();
    void chatTabChanged();
    void contactPaneChanged();
    void currentContactChanged();
    void currentUserChanged();
    void currentChatPeerChanged();
    void currentDialogUidChanged();
    void searchPendingChanged();
    void searchStatusChanged();
    void pendingApplyChanged();
    void chatLoadingMoreChanged();
    void canLoadMoreChatsChanged();
    void privateHistoryLoadingChanged();
    void canLoadMorePrivateHistoryChanged();
    void contactLoadingMoreChanged();
    void canLoadMoreContactsChanged();
    void authStatusChanged();
    void settingsStatusChanged();
    void currentGroupChanged();
    void groupStatusChanged();
    void mediaUploadStateChanged();
    void currentDraftTextChanged();
    void currentPendingAttachmentsChanged();
    void currentDialogPinnedChanged();
    void currentDialogMutedChanged();
    void pendingReplyChanged();
    void lazyBootstrapStateChanged();
    void groupCreated(qint64 groupId);

private slots:
    void onLoginHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onResetHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onSwitchToChat();
    void onRelationBootstrapUpdated();
    void onRegisterCountdownTimeout();
    void onRegisterCodeCooldownTimeout();
    void onAddAuthFriend(std::shared_ptr<AuthInfo> authInfo);
    void onDeleteFriendRsp(int error, int friendUid);
    void onAuthRsp(std::shared_ptr<AuthRsp> authRsp);
    void onTextChatMsg(std::shared_ptr<TextChatMsg> msg);
    void onUserSearch(std::shared_ptr<SearchInfo> searchInfo);
    void onFriendApply(std::shared_ptr<AddFriendApply> applyInfo);
    void onGroupListUpdated();
    void onGroupInvite(qint64 groupId, QString groupCode, QString groupName, int operatorUid);
    void onGroupApply(qint64 groupId, int applicantUid, QString applicantUserId, QString reason);
    void onGroupMemberChanged(QJsonObject payload);
    void onGroupChatMsg(std::shared_ptr<GroupChatMsg> msg);
    void onGroupRsp(ReqId reqId, int error, QJsonObject payload);
    void onDialogListRsp(QJsonObject payload);
    void onPrivateHistoryRsp(QJsonObject payload);
    void onPrivateMsgChanged(QJsonObject payload);
    void onPrivateReadAck(QJsonObject payload);
    void onMessageStatus(QJsonObject payload);

private:
    friend class AppSessionCoordinator;
    friend class SessionAuthCoordinator;
    friend class SessionChatEntryCoordinator;
    friend class SessionRelationBootstrap;
    friend class RegisterCountdownController;
    friend class ContactCoordinatorShell;
    friend class GroupCoordinator;
    friend class MediaCoordinator;
    friend class CallCoordinator;
    friend class ProfileCoordinator;
    friend class AppChatConnectionCoordinator;

    bool isChatTransportReady() const;
    bool dispatchChatContent(const QString& content, const QString& previewText);
    bool dispatchGroupChatContent(const QString& content, const QString& previewText);
    void startMediaUploadAndSend(const QString& fileUrl, const QString& mediaType, const QString& fallbackName);
    void addPendingAttachments(const QVariantList& attachments);
    void removePendingAttachmentById(const QString& attachmentId, int dialogUid = 0);
    void setCurrentPendingAttachments(const QVariantList& attachments);
    void processPendingAttachmentQueue();
    bool sendUploadedAttachmentToDialog(const QVariantMap& attachment,
                                        const UploadedMediaInfo& uploaded,
                                        int dialogUid,
                                        int chatUid,
                                        qint64 groupId);
    void setTip(const QString& tip, bool isError);
    void setBusy(bool value);
    void setRegisterSuccessPage(bool success);
    void setRegisterCountdown(int seconds);
    void setRegisterCodeCooldownSeconds(int seconds);
    void setRegisterCodeRequestPending(bool pending);
    void setPage(Page newPage);
    QString normalizeIconPath(QString icon) const;
    void applyCurrentUserProfile(const QJsonObject& profile, bool preserveExistingIcon);
    void applyCurrentUserProfile(int uid,
                                 const QString& name,
                                 const QString& nick,
                                 const QString& icon,
                                 const QString& desc,
                                 const QString& userId,
                                 int sex,
                                 bool preserveExistingIcon);
    void refreshFriendModels();
    void refreshApplyModel();
    void refreshGroupModel();
    void refreshDialogModel();
    void refreshDialogModelIncremental();
    void requestDialogList();
    void requestRelationBootstrap();
    void bootstrapDialogs();
    void bootstrapContacts();
    void bootstrapGroups();
    void bootstrapApplies();
    void setDialogsReady(bool ready);
    void setContactsReady(bool ready);
    void setGroupsReady(bool ready);
    void setApplyReady(bool ready);
    void refreshChatLoadMoreState();
    void refreshContactLoadMoreState();
    void loadCurrentChatMessages();
    void selectGroupByDialogUid(int dialogUid, qint64 groupId);
    void requestPrivateHistory(int peerUid, qint64 beforeTs, const QString& beforeMsgId = QString());
    void requestPrivateHistoryForBootstrap(int peerUid);
    void requestGroupHistoryForBootstrap(qint64 groupId);
    void setContactPane(ContactPane pane);
    void setCurrentContact(int uid,
                           const QString& name,
                           const QString& nick,
                           const QString& icon,
                           const QString& back,
                           int sex,
                           const QString& userId = QString());
    void setCurrentChatPeerName(const QString& name);
    void setCurrentChatPeerIcon(const QString& icon);
    void selectChatByUid(int uid);
    void setSearchPending(bool pending);
    void setSearchStatus(const QString& text, bool isError);
    void clearSearchResultOnly();
    void setChatLoadingMore(bool loading);
    void setPrivateHistoryLoading(bool loading);
    void setCanLoadMorePrivateHistory(bool canLoad);
    void setContactLoadingMore(bool loading);
    void setAuthStatus(const QString& text, bool isError);
    void setSettingsStatus(const QString& text, bool isError);
    void setCurrentGroup(qint64 groupId, const QString& name, const QString& groupCode = QString());
    void setGroupStatus(const QString& text, bool isError);
    void setMediaUploadInProgress(bool inProgress);
    void setMediaUploadProgressText(const QString& text);
    void setCurrentDraftText(const QString& text);
    void setCurrentDialogPinned(bool pinned);
    void setCurrentDialogMuted(bool muted);
    void setPendingReplyContext(const QString& msgId, const QString& senderName, const QString& previewText);
    int currentDialogUid() const;
    void emitCurrentDialogUidChangedIfNeeded();
    bool resolveDialogTarget(int dialogUid, QString& dialogType, int& peerUid, qint64& groupId) const;
    qint64 currentGroupPermissionBitsRaw() const;
    bool hasCurrentGroupPermission(qint64 permissionBit) const;
    qint64 latestGroupCreatedAt(qint64 groupId) const;
    qint64 latestPrivatePeerCreatedAt(int peerUid) const;
    void syncCurrentDialogDraft();
    void clearCurrentGroupConversation(qint64 groupId);
    void loadDraftStore(int ownerUid);
    void saveDraftStore(int ownerUid) const;
    void applyDraftToDialogModel(int dialogUid, const QString& draftText);
    void syncCurrentPendingAttachments();
    void sendGroupReadAck(qint64 groupId, qint64 readTs = 0);
    void sendPrivateReadAck(int peerUid, qint64 readTs = 0);
    void runPostLoginBootstrap();
    bool handleGroupRspError(ReqId reqId, int error, const QJsonObject& payload);
    void handleGroupManagementRsp(ReqId reqId, const QJsonObject& payload);
    void handleGroupHistoryRsp(const QJsonObject& payload);
    void handleGroupMessageAckRsp(ReqId reqId, const QJsonObject& payload);
    void handleGroupMessageMutationRsp(ReqId reqId, const QJsonObject& payload);
    void handlePrivateMessageMutationRsp(ReqId reqId, const QJsonObject& payload);
    void handlePrivateForwardRsp(const QJsonObject& payload);
    void handleDialogMetaRsp(ReqId reqId, const QJsonObject& payload);
    void syncChatViewModelState();
    void syncContactControllerState();
    void syncGroupControllerState();
    void syncCallControllerState();
    void syncShellViewModelState();

    Page _page;
    ChatTab _chat_tab;
    ContactPane _contact_pane;
    AppPendingLoginState _pending_login_state;
    AppChatEndpointState _chat_endpoint_state;
    AppChatRecoveryState _chat_recovery_state;
    AppShellRuntimeState _shell_state;
    AppSearchRuntimeState _search_state;
    AppFeatureStatusState _feature_status_state;
    AppLoadingRuntimeState _loading_state;
    AppLazyBootstrapState _bootstrap_state;
    AppMediaUploadRuntimeState _media_upload_state;

    AppCurrentUserState _user_state;
    AppCurrentContactState _contact_state;
    AppCurrentChatState _chat_state;
    AppGroupRuntimeState _group_state;

    FriendListModel _chat_list_model;
    FriendListModel _group_list_model;
    FriendListModel _dialog_list_model;
    FriendListModel _contact_list_model;
    ChatMessageModel _message_model;
    SearchResultModel _search_result_model;
    ApplyRequestModel _apply_request_model;
    AppDialogRuntimeState _dialog_state;
    AppPrivateHistoryState _private_history_state;

    ClientGateway _gateway;
    ShellViewModel _shell_view_model;
    AuthController _auth_controller;
    AuthService _auth_service;
    AuthViewModel _auth_view_model;
    CallController _call_controller;
    CallSessionModel _call_session_model;
    LivekitBridge _livekit_bridge;
    ChatController _chat_controller;
    ChatViewModel _chat_view_model;
    ContactController _contact_controller;
    GroupController _group_controller;
    ProfileController _profile_controller;
    MomentsController _moments_controller;
    AgentController _agent_controller;
    PetController _pet_controller;
    R18Controller _r18_controller;
    PrivateChatCacheStore _private_cache_store;
    GroupChatCacheStore _group_cache_store;
    QTimer _register_countdown_timer;
    QTimer _register_code_cooldown_timer;
    QTimer _heartbeat_timer;
    QTimer _chat_login_timeout_timer;
    AppPendingSendQueueState _pending_send_state;

    std::unique_ptr<AppSessionCoordinator> _session_coordinator;
    std::unique_ptr<ContactCoordinatorShell> _contact_coordinator_shell;
    std::unique_ptr<GroupCoordinator> _group_coordinator;
    std::unique_ptr<MediaCoordinator> _media_coordinator;
    std::unique_ptr<CallCoordinator> _call_coordinator;
    std::unique_ptr<ProfileCoordinator> _profile_coordinator;
    std::unique_ptr<AppChatConnectionCoordinator> _chat_connection_coordinator;
};

#endif // APPCONTROLLER_H

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
#include "CallController.h"
#include "CallSessionModel.h"
#include "ChatController.h"
#include "ClientGateway.h"
#include "ContactController.h"
#include "FriendListModel.h"
#include "ChatMessageModel.h"
#include "SearchResultModel.h"
#include "ApplyRequestModel.h"
#include "ProfileController.h"
#include "MomentsController.h"
#include "MomentsModel.h"
#include "PrivateChatCacheStore.h"
#include "GroupChatCacheStore.h"
#include "LivekitBridge.h"
#include "MediaUploadService.h"

class AppSessionCoordinator;
class ContactCoordinatorShell;
class GroupCoordinator;
class MediaCoordinator;
class CallCoordinator;
class ProfileCoordinator;

class AppController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(Page page READ page NOTIFY pageChanged)
    Q_PROPERTY(QString tipText READ tipText NOTIFY tipChanged)
    Q_PROPERTY(bool tipError READ tipError NOTIFY tipChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool registerSuccessPage READ registerSuccessPage NOTIFY registerSuccessPageChanged)
    Q_PROPERTY(int registerCountdown READ registerCountdown NOTIFY registerCountdownChanged)
    Q_PROPERTY(ChatTab chatTab READ chatTab NOTIFY chatTabChanged)
    Q_PROPERTY(QString currentUserName READ currentUserName NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserNick READ currentUserNick NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserIcon READ currentUserIcon NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserDesc READ currentUserDesc NOTIFY currentUserChanged)
    Q_PROPERTY(QString currentUserId READ currentUserId NOTIFY currentUserChanged)
    Q_PROPERTY(ContactPane contactPane READ contactPane NOTIFY contactPaneChanged)
    Q_PROPERTY(QString currentContactName READ currentContactName NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactNick READ currentContactNick NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactIcon READ currentContactIcon NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactBack READ currentContactBack NOTIFY currentContactChanged)
    Q_PROPERTY(int currentContactSex READ currentContactSex NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentContactUserId READ currentContactUserId NOTIFY currentContactChanged)
    Q_PROPERTY(bool hasCurrentContact READ hasCurrentContact NOTIFY currentContactChanged)
    Q_PROPERTY(QString currentChatPeerName READ currentChatPeerName NOTIFY currentChatPeerChanged)
    Q_PROPERTY(QString currentChatPeerIcon READ currentChatPeerIcon NOTIFY currentChatPeerChanged)
    Q_PROPERTY(bool hasCurrentChat READ hasCurrentChat NOTIFY currentChatPeerChanged)
    Q_PROPERTY(int currentDialogUid READ currentDialogUid NOTIFY currentDialogUidChanged)
    Q_PROPERTY(bool hasCurrentGroup READ hasCurrentGroup NOTIFY currentGroupChanged)
    Q_PROPERTY(int currentGroupRole READ currentGroupRole NOTIFY currentGroupChanged)
    Q_PROPERTY(QString currentGroupName READ currentGroupName NOTIFY currentGroupChanged)
    Q_PROPERTY(QString currentGroupCode READ currentGroupCode NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanChangeInfo READ currentGroupCanChangeInfo NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanInviteUsers READ currentGroupCanInviteUsers NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanManageAdmins READ currentGroupCanManageAdmins NOTIFY currentGroupChanged)
    Q_PROPERTY(bool currentGroupCanBanUsers READ currentGroupCanBanUsers NOTIFY currentGroupChanged)
    Q_PROPERTY(FriendListModel* dialogListModel READ dialogListModel CONSTANT)
    Q_PROPERTY(FriendListModel* chatListModel READ chatListModel CONSTANT)
    Q_PROPERTY(FriendListModel* groupListModel READ groupListModel CONSTANT)
    Q_PROPERTY(FriendListModel* contactListModel READ contactListModel CONSTANT)
    Q_PROPERTY(ChatMessageModel* messageModel READ messageModel CONSTANT)
    Q_PROPERTY(SearchResultModel* searchResultModel READ searchResultModel CONSTANT)
    Q_PROPERTY(ApplyRequestModel* applyRequestModel READ applyRequestModel CONSTANT)
    Q_PROPERTY(MomentsModel* momentsModel READ momentsModel CONSTANT)
    Q_PROPERTY(MomentsController* momentsController READ momentsController CONSTANT)
    Q_PROPERTY(bool searchPending READ searchPending NOTIFY searchPendingChanged)
    Q_PROPERTY(QString searchStatusText READ searchStatusText NOTIFY searchStatusChanged)
    Q_PROPERTY(bool searchStatusError READ searchStatusError NOTIFY searchStatusChanged)
    Q_PROPERTY(bool hasPendingApply READ hasPendingApply NOTIFY pendingApplyChanged)
    Q_PROPERTY(bool chatLoadingMore READ chatLoadingMore NOTIFY chatLoadingMoreChanged)
    Q_PROPERTY(bool canLoadMoreChats READ canLoadMoreChats NOTIFY canLoadMoreChatsChanged)
    Q_PROPERTY(bool privateHistoryLoading READ privateHistoryLoading NOTIFY privateHistoryLoadingChanged)
    Q_PROPERTY(bool canLoadMorePrivateHistory READ canLoadMorePrivateHistory NOTIFY canLoadMorePrivateHistoryChanged)
    Q_PROPERTY(bool contactLoadingMore READ contactLoadingMore NOTIFY contactLoadingMoreChanged)
    Q_PROPERTY(bool canLoadMoreContacts READ canLoadMoreContacts NOTIFY canLoadMoreContactsChanged)
    Q_PROPERTY(QString authStatusText READ authStatusText NOTIFY authStatusChanged)
    Q_PROPERTY(bool authStatusError READ authStatusError NOTIFY authStatusChanged)
    Q_PROPERTY(QString settingsStatusText READ settingsStatusText NOTIFY settingsStatusChanged)
    Q_PROPERTY(bool settingsStatusError READ settingsStatusError NOTIFY settingsStatusChanged)
    Q_PROPERTY(QString groupStatusText READ groupStatusText NOTIFY groupStatusChanged)
    Q_PROPERTY(bool groupStatusError READ groupStatusError NOTIFY groupStatusChanged)
    Q_PROPERTY(bool mediaUploadInProgress READ mediaUploadInProgress NOTIFY mediaUploadStateChanged)
    Q_PROPERTY(QString mediaUploadProgressText READ mediaUploadProgressText NOTIFY mediaUploadStateChanged)
    Q_PROPERTY(CallSessionModel* callSession READ callSession CONSTANT)
    Q_PROPERTY(LivekitBridge* livekitBridge READ livekitBridge CONSTANT)
    Q_PROPERTY(QString currentDraftText READ currentDraftText NOTIFY currentDraftTextChanged)
    Q_PROPERTY(QVariantList currentPendingAttachments READ currentPendingAttachments NOTIFY currentPendingAttachmentsChanged)
    Q_PROPERTY(bool hasPendingAttachments READ hasPendingAttachments NOTIFY currentPendingAttachmentsChanged)
    Q_PROPERTY(bool currentDialogPinned READ currentDialogPinned NOTIFY currentDialogPinnedChanged)
    Q_PROPERTY(bool currentDialogMuted READ currentDialogMuted NOTIFY currentDialogMutedChanged)
    Q_PROPERTY(bool hasPendingReply READ hasPendingReply NOTIFY pendingReplyChanged)
    Q_PROPERTY(QString replyTargetName READ replyTargetName NOTIFY pendingReplyChanged)
    Q_PROPERTY(QString replyPreviewText READ replyPreviewText NOTIFY pendingReplyChanged)
    Q_PROPERTY(bool dialogsReady READ dialogsReady NOTIFY lazyBootstrapStateChanged)
    Q_PROPERTY(bool contactsReady READ contactsReady NOTIFY lazyBootstrapStateChanged)
    Q_PROPERTY(bool groupsReady READ groupsReady NOTIFY lazyBootstrapStateChanged)
    Q_PROPERTY(bool applyReady READ applyReady NOTIFY lazyBootstrapStateChanged)
    Q_PROPERTY(bool chatShellBusy READ chatShellBusy NOTIFY lazyBootstrapStateChanged)

public:
    enum Page {
        LoginPage = 0,
        RegisterPage = 1,
        ResetPage = 2,
        ChatPage = 3
    };
    Q_ENUM(Page)

    enum ChatTab {
        ChatTabPage = 0,
        ContactTabPage = 1,
        SettingsTabPage = 2,
        MomentsTabPage = 3
    };
    Q_ENUM(ChatTab)

    enum ContactPane {
        ApplyRequestPane = 0,
        FriendInfoPane = 1
    };
    Q_ENUM(ContactPane)

    explicit AppController(QObject *parent = nullptr);
    ~AppController();

    Page page() const;
    QString tipText() const;
    bool tipError() const;
    bool busy() const;
    bool registerSuccessPage() const;
    int registerCountdown() const;
    ChatTab chatTab() const;
    ContactPane contactPane() const;
    QString currentUserName() const;
    QString currentUserNick() const;
    QString currentUserIcon() const;
    QString currentUserDesc() const;
    QString currentUserId() const;
    QString currentContactName() const;
    QString currentContactNick() const;
    QString currentContactIcon() const;
    QString currentContactBack() const;
    int currentContactSex() const;
    QString currentContactUserId() const;
    bool hasCurrentContact() const;
    QString currentChatPeerName() const;
    QString currentChatPeerIcon() const;
    bool hasCurrentChat() const;
    bool hasCurrentGroup() const;
    int currentGroupRole() const;
    QString currentGroupName() const;
    QString currentGroupCode() const;
    bool currentGroupCanChangeInfo() const;
    bool currentGroupCanInviteUsers() const;
    bool currentGroupCanManageAdmins() const;
    bool currentGroupCanBanUsers() const;
    FriendListModel *dialogListModel();
    FriendListModel *chatListModel();
    FriendListModel *groupListModel();
    FriendListModel *contactListModel();
    ChatMessageModel *messageModel();
    SearchResultModel *searchResultModel();
    ApplyRequestModel *applyRequestModel();
    MomentsModel *momentsModel() const;
    MomentsController *momentsController() const;
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
    CallSessionModel *callSession();
    LivekitBridge *livekitBridge();
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

    Q_INVOKABLE void switchToLogin();
    Q_INVOKABLE void switchToRegister();
    Q_INVOKABLE void switchToReset();
    Q_INVOKABLE void switchChatTab(int tab);
    Q_INVOKABLE void ensureContactsInitialized();
    Q_INVOKABLE void ensureGroupsInitialized();
    Q_INVOKABLE void ensureApplyInitialized();
    Q_INVOKABLE void ensureChatListInitialized();
    Q_INVOKABLE void clearTip();
    Q_INVOKABLE void selectChatIndex(int index);
    Q_INVOKABLE void selectGroupIndex(int index);
    Q_INVOKABLE void selectDialogByUid(int uid);
    Q_INVOKABLE void selectContactIndex(int index);
    Q_INVOKABLE void showApplyRequests();
    Q_INVOKABLE void jumpChatWithCurrentContact();
    Q_INVOKABLE void loadMoreChats();
    Q_INVOKABLE void loadMorePrivateHistory();
    Q_INVOKABLE void loadMoreContacts();
    Q_INVOKABLE void sendTextMessage(const QString &text);
    Q_INVOKABLE void sendCurrentComposerPayload(const QString &text);
    Q_INVOKABLE void sendImageMessage();
    Q_INVOKABLE void sendFileMessage();
    Q_INVOKABLE void removePendingAttachment(const QString &attachmentId);
    Q_INVOKABLE void clearPendingAttachments();
    Q_INVOKABLE void openExternalResource(const QString &url);
    Q_INVOKABLE void searchUser(const QString &uidText);
    Q_INVOKABLE void clearSearchState();
    Q_INVOKABLE void requestAddFriend(int uid, const QString &bakName, const QVariantList &labels = QVariantList());
    Q_INVOKABLE void approveFriend(int uid, const QString &backName, const QVariantList &labels = QVariantList());
    Q_INVOKABLE void clearAuthStatus();
    Q_INVOKABLE void startVoiceChat();
    Q_INVOKABLE void startVideoChat();
    Q_INVOKABLE void acceptIncomingCall();
    Q_INVOKABLE void rejectIncomingCall();
    Q_INVOKABLE void endCurrentCall();
    Q_INVOKABLE void toggleCallMuted();
    Q_INVOKABLE void toggleCallCamera();
    Q_INVOKABLE void chooseAvatar(int source = 0);
    Q_INVOKABLE void saveProfile(const QString &nick, const QString &desc);
    Q_INVOKABLE void clearSettingsStatus();
    Q_INVOKABLE void refreshGroupList();
    Q_INVOKABLE void createGroup(const QString &name, const QVariantList &memberUserIdList = QVariantList());
    Q_INVOKABLE void inviteGroupMember(const QString &userId, const QString &reason = QString());
    Q_INVOKABLE void applyJoinGroup(const QString &groupCode, const QString &reason = QString());
    Q_INVOKABLE void reviewGroupApply(qint64 applyId, bool agree);
    Q_INVOKABLE void sendGroupTextMessage(const QString &text);
    Q_INVOKABLE void sendGroupImageMessage();
    Q_INVOKABLE void sendGroupFileMessage();
    Q_INVOKABLE void editGroupMessage(const QString &msgId, const QString &text);
    Q_INVOKABLE void revokeGroupMessage(const QString &msgId);
    Q_INVOKABLE void forwardGroupMessage(const QString &msgId);
    Q_INVOKABLE void loadGroupHistory();
    Q_INVOKABLE void updateGroupAnnouncement(const QString &announcement);
    Q_INVOKABLE void updateGroupIcon(int source = 0);
    Q_INVOKABLE void setGroupAdmin(const QString &userId, bool isAdmin, qint64 permissionBits = 0);
    Q_INVOKABLE void muteGroupMember(const QString &userId, int muteSeconds);
    Q_INVOKABLE void kickGroupMember(const QString &userId);
    Q_INVOKABLE void quitCurrentGroup();
    Q_INVOKABLE void clearGroupStatus();
    Q_INVOKABLE void updateCurrentDraft(const QString &text);
    Q_INVOKABLE void toggleCurrentDialogPinned();
    Q_INVOKABLE void toggleCurrentDialogMuted();
    Q_INVOKABLE void toggleDialogPinnedByUid(int dialogUid);
    Q_INVOKABLE void toggleDialogMutedByUid(int dialogUid);
    Q_INVOKABLE void markDialogReadByUid(int dialogUid);
    Q_INVOKABLE void clearDialogDraftByUid(int dialogUid);
    Q_INVOKABLE void beginReplyMessage(const QString &msgId, const QString &senderName, const QString &previewText);
    Q_INVOKABLE void cancelReplyMessage();

    Q_INVOKABLE void login(const QString &email, const QString &password);
    Q_INVOKABLE void requestRegisterCode(const QString &email);
    Q_INVOKABLE void registerUser(const QString &user, const QString &email, const QString &password,
                                  const QString &confirm, const QString &verifyCode);
    Q_INVOKABLE void requestResetCode(const QString &email);
    Q_INVOKABLE void resetPassword(const QString &user, const QString &email, const QString &password,
                                   const QString &verifyCode);

signals:
    void pageChanged();
    void tipChanged();
    void busyChanged();
    void registerSuccessPageChanged();
    void registerCountdownChanged();
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

private slots:
    void onLoginHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onResetHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onCallHttpFinished(ReqId id, QString res, ErrorCodes err);
    void onTcpConnectFinished(bool success);
    void onChatLoginFailed(int err);
    void onSwitchToChat();
    void onRelationBootstrapUpdated();
    void onRegisterCountdownTimeout();
    void onHeartbeatTimeout();
    void onHeartbeatAck(qint64 ackAtMs);
    void onAddAuthFriend(std::shared_ptr<AuthInfo> authInfo);
    void onAuthRsp(std::shared_ptr<AuthRsp> authRsp);
    void onTextChatMsg(std::shared_ptr<TextChatMsg> msg);
    void onUserSearch(std::shared_ptr<SearchInfo> searchInfo);
    void onFriendApply(std::shared_ptr<AddFriendApply> applyInfo);
    void onNotifyOffline();
    void onConnectionClosed();
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
    void onCallEvent(QJsonObject payload);
    void onLivekitRoomJoined();
    void onLivekitRemoteTrackReady();
    void onLivekitRoomDisconnected(const QString &reason, bool recoverable);
    void onLivekitPermissionError(const QString &deviceType, const QString &message);
    void onLivekitMediaError(const QString &message);
    void onLivekitReconnecting(const QString &message);

private:
    friend class AppSessionCoordinator;
    friend class ContactCoordinatorShell;
    friend class GroupCoordinator;
    friend class MediaCoordinator;
    friend class CallCoordinator;
    friend class ProfileCoordinator;

    bool parseJson(const QString &res, QJsonObject &obj);
    bool checkEmailValid(const QString &email);
    bool checkPwdValid(const QString &password);
    bool checkUserValid(const QString &user);
    bool checkVerifyCodeValid(const QString &code);
    bool isChatTransportReady() const;
    bool dispatchChatContent(const QString &content, const QString &previewText);
    bool dispatchGroupChatContent(const QString &content, const QString &previewText);
    void startMediaUploadAndSend(const QString &fileUrl, const QString &mediaType, const QString &fallbackName);
    void addPendingAttachments(const QVariantList &attachments);
    void removePendingAttachmentById(const QString &attachmentId, int dialogUid = 0);
    void setCurrentPendingAttachments(const QVariantList &attachments);
    void processPendingAttachmentQueue();
    bool sendUploadedAttachmentToDialog(const QVariantMap &attachment, const UploadedMediaInfo &uploaded,
                                        int dialogUid, int chatUid, qint64 groupId);
    void sendCallInvite(const QString &callType);
    bool ensureCallTargetFromCurrentChat();
    void startCallFlow(const QString &callType);
    void finalizeEndedCall(const QString &statusText);
    void setTip(const QString &tip, bool isError);
    void setBusy(bool value);
    void setPage(Page newPage);
    QString normalizeIconPath(QString icon) const;
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
    void requestPrivateHistory(int peerUid, qint64 beforeTs, const QString &beforeMsgId = QString());
    void setContactPane(ContactPane pane);
    void setCurrentContact(int uid, const QString &name, const QString &nick, const QString &icon,
                           const QString &back, int sex, const QString &userId = QString());
    void setCurrentChatPeerName(const QString &name);
    void setCurrentChatPeerIcon(const QString &icon);
    void selectChatByUid(int uid);
    void setSearchPending(bool pending);
    void setSearchStatus(const QString &text, bool isError);
    void clearSearchResultOnly();
    void setChatLoadingMore(bool loading);
    void setPrivateHistoryLoading(bool loading);
    void setCanLoadMorePrivateHistory(bool canLoad);
    void setContactLoadingMore(bool loading);
    void setAuthStatus(const QString &text, bool isError);
    void setSettingsStatus(const QString &text, bool isError);
    void setCurrentGroup(qint64 groupId, const QString &name, const QString &groupCode = QString());
    void setGroupStatus(const QString &text, bool isError);
    void setMediaUploadInProgress(bool inProgress);
    void setMediaUploadProgressText(const QString &text);
    void setCurrentDraftText(const QString &text);
    void setCurrentDialogPinned(bool pinned);
    void setCurrentDialogMuted(bool muted);
    void setPendingReplyContext(const QString &msgId, const QString &senderName, const QString &previewText);
    int currentDialogUid() const;
    void emitCurrentDialogUidChangedIfNeeded();
    bool resolveDialogTarget(int dialogUid, QString &dialogType, int &peerUid, qint64 &groupId) const;
    qint64 currentGroupPermissionBitsRaw() const;
    bool hasCurrentGroupPermission(qint64 permissionBit) const;
    qint64 latestGroupCreatedAt(qint64 groupId) const;
    qint64 latestPrivatePeerCreatedAt(int peerUid) const;
    void syncCurrentDialogDraft();
    void loadDraftStore(int ownerUid);
    void saveDraftStore(int ownerUid) const;
    void applyDraftToDialogModel(int dialogUid, const QString &draftText);
    void syncCurrentPendingAttachments();
    void sendGroupReadAck(qint64 groupId, qint64 readTs = 0);
    void sendPrivateReadAck(int peerUid, qint64 readTs = 0);
    bool tryReconnectChat();
    void resetReconnectState();
    void resetHeartbeatTracking();
    bool isHeartbeatLikelyTimeout() const;

    Page _page;
    QString _tip_text;
    bool _tip_error;
    bool _busy;
    bool _register_success_page;
    int _register_countdown;
    ChatTab _chat_tab;
    ContactPane _contact_pane;
    int _pending_uid;
    QString _pending_token;
    QString _pending_login_ticket;
    QString _pending_trace_id;
    QString _chat_server_host;
    QString _chat_server_port;
    QString _chat_server_name;
    QVector<ChatEndpoint> _chat_endpoints;
    int _chat_endpoint_index = -1;
    int _chat_connect_timeout_ms = 1200;
    int _chat_backup_dial_delay_ms = 250;
    int _chat_total_login_timeout_ms = 5000;
    int _chat_protocol_version = 2;
    qint64 _login_started_ms = 0;
    qint64 _http_login_finished_ms = 0;
    qint64 _chat_connect_started_ms = 0;
    qint64 _chat_connect_finished_ms = 0;

    QString _current_user_name;
    QString _current_user_nick;
    QString _current_user_icon;
    QString _current_user_desc;
    QString _current_user_id;
    int _current_contact_uid;
    QString _current_contact_name;
    QString _current_contact_nick;
    QString _current_contact_icon;
    QString _current_contact_back;
    int _current_contact_sex;
    QString _current_contact_user_id;
    QString _current_chat_peer_name;
    QString _current_chat_peer_icon;
    int _current_chat_uid;
    qint64 _current_group_id;
    QString _current_group_name;
    QString _current_group_code;

    FriendListModel _chat_list_model;
    FriendListModel _group_list_model;
    FriendListModel _dialog_list_model;
    FriendListModel _contact_list_model;
    ChatMessageModel _message_model;
    SearchResultModel _search_result_model;
    ApplyRequestModel _apply_request_model;
    bool _search_pending;
    QString _search_status_text;
    bool _search_status_error;
    bool _chat_loading_more;
    bool _can_load_more_chats;
    bool _private_history_loading;
    bool _can_load_more_private_history;
    bool _contact_loading_more;
    bool _can_load_more_contacts;
    QString _auth_status_text;
    bool _auth_status_error;
    QString _settings_status_text;
    bool _settings_status_error;
    QString _group_status_text;
    bool _group_status_error;
    bool _media_upload_in_progress = false;
    bool _settings_avatar_upload_in_progress = false;
    bool _group_icon_upload_in_progress = false;
    QString _media_upload_progress_text;
    QString _current_draft_text;
    QVariantList _current_pending_attachments;
    QHash<int, QVariantList> _dialog_pending_attachment_map;
    bool _current_dialog_pinned = false;
    bool _current_dialog_muted = false;
    QString _reply_to_msg_id;
    QString _reply_target_name;
    QString _reply_preview_text;
    QMap<int, qint64> _group_uid_map;
    QHash<QString, qint64> _pending_group_msg_group_map;
    QHash<int, QString> _dialog_draft_map;
    QSet<int> _dialog_local_pinned_set;
    QHash<int, int> _dialog_server_pinned_map;
    QHash<int, int> _dialog_server_mute_map;
    QHash<int, int> _dialog_mention_map;
    qint64 _private_history_before_ts;
    QString _private_history_before_msg_id;
    qint64 _private_history_pending_before_ts;
    QString _private_history_pending_before_msg_id;
    int _private_history_pending_peer_uid;
    qint64 _group_history_before_seq;
    bool _group_history_has_more;
    bool _dialog_bootstrap_loading = false;
    bool _dialogs_ready = false;
    bool _contacts_ready = false;
    bool _groups_ready = false;
    bool _apply_ready = false;
    bool _chat_list_initialized = false;

    ClientGateway _gateway;
    AuthController _auth_controller;
    CallController _call_controller;
    CallSessionModel _call_session_model;
    LivekitBridge _livekit_bridge;
    ChatController _chat_controller;
    ContactController _contact_controller;
    ProfileController _profile_controller;
    MomentsController _moments_controller;
    PrivateChatCacheStore _private_cache_store;
    GroupChatCacheStore _group_cache_store;
    QTimer _register_countdown_timer;
    QTimer _heartbeat_timer;
    QTimer _chat_login_timeout_timer;
    qint64 _last_heartbeat_sent_ms = 0;
    qint64 _last_heartbeat_ack_ms = 0;
    int _heartbeat_ack_miss_count = 0;
    bool _closed_by_heartbeat_watchdog = false;
    bool _reconnecting_chat = false;
    int _chat_reconnect_attempts = 0;
    bool _ignore_next_login_disconnect = false;
    int _last_emitted_dialog_uid = 0;
    QVariantList _pending_send_queue;
    int _pending_send_dialog_uid = 0;
    int _pending_send_chat_uid = 0;
    qint64 _pending_send_group_id = 0;
    int _pending_send_total_count = 0;

    std::unique_ptr<AppSessionCoordinator> _session_coordinator;
    std::unique_ptr<ContactCoordinatorShell> _contact_coordinator_shell;
    std::unique_ptr<GroupCoordinator> _group_coordinator;
    std::unique_ptr<MediaCoordinator> _media_coordinator;
    std::unique_ptr<CallCoordinator> _call_coordinator;
    std::unique_ptr<ProfileCoordinator> _profile_coordinator;
};

#endif // APPCONTROLLER_H

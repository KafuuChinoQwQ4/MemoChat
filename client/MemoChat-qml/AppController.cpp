#include "AppController.h"
#include "AppCoordinators.h"
#include "ChatMessageDispatcher.h"
#include "DialogListService.h"
#include "LocalFilePickerService.h"
#include "MediaUploadService.h"
#include "MessageContentCodec.h"
#include "IconPathUtils.h"
#include "IChatTransport.h"
#include "httpmgr.h"
#include "usermgr.h"
#include <QDateTime>
#include <algorithm>
#include <QFileInfo>
#include <QUuid>
#include <QRegularExpression>
#include <QUrl>
#include <QSettings>
#include <QVariantMap>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QtGlobal>
#include <QDebug>

namespace {
struct MediaUploadTaskResult {
    bool ok = false;
    UploadedMediaInfo uploaded;
    QString errorText;
};

constexpr qint64 kPermChangeGroupInfo = 1LL << 0;
constexpr qint64 kPermDeleteMessages = 1LL << 1;
constexpr qint64 kPermInviteUsers = 1LL << 2;
constexpr qint64 kPermManageAdmins = 1LL << 3;
constexpr qint64 kPermPinMessages = 1LL << 4;
constexpr qint64 kPermBanUsers = 1LL << 5;
constexpr qint64 kPermManageTopics = 1LL << 6;
constexpr qint64 kDefaultAdminPermBits =
    kPermChangeGroupInfo | kPermDeleteMessages | kPermInviteUsers | kPermPinMessages | kPermBanUsers;
constexpr qint64 kOwnerPermBits = kDefaultAdminPermBits | kPermManageAdmins | kPermManageTopics;
constexpr int kHeartbeatIntervalMs = 10000;
constexpr int kHeartbeatAckMissThreshold = 2;
constexpr qint64 kHeartbeatAckGraceMs = 22000;
constexpr int kChatReconnectMaxAttempts = 2;
constexpr int kChatReconnectDelayMs = 300;

}

AppController::AppController(QObject *parent)
    : QObject(parent),
      _page(LoginPage),
      _tip_text(""),
      _tip_error(false),
      _busy(false),
      _register_success_page(false),
      _register_countdown(5),
      _chat_tab(ChatTabPage),
      _contact_pane(ApplyRequestPane),
      _pending_uid(0),
      _current_user_icon("qrc:/res/head_1.jpg"),
      _current_contact_uid(0),
      _current_contact_sex(0),
      _current_contact_icon("qrc:/res/head_1.jpg"),
      _current_chat_peer_icon("qrc:/res/head_1.jpg"),
      _current_chat_uid(0),
      _current_group_id(0),
      _chat_list_model(this),
      _group_list_model(this),
      _dialog_list_model(this),
      _contact_list_model(this),
      _message_model(this),
      _search_result_model(this),
      _apply_request_model(this),
      _search_pending(false),
      _search_status_error(false),
      _chat_loading_more(false),
      _can_load_more_chats(false),
      _private_history_loading(false),
      _can_load_more_private_history(false),
      _contact_loading_more(false),
      _can_load_more_contacts(false),
      _auth_status_error(false),
      _settings_status_error(false),
      _group_status_error(false),
      _private_history_before_ts(0),
      _private_history_before_msg_id(),
      _private_history_pending_before_ts(0),
      _private_history_pending_before_msg_id(),
      _private_history_pending_peer_uid(0),
      _group_history_before_seq(0),
      _group_history_has_more(true),
      _auth_controller(&_gateway),
      _call_controller(&_gateway),
      _call_session_model(this),
      _livekit_bridge(this),
      _chat_controller(&_gateway),
      _contact_controller(&_gateway),
      _profile_controller(&_gateway),
      _session_coordinator(std::make_unique<AppSessionCoordinator>(*this)),
      _contact_coordinator_shell(std::make_unique<ContactCoordinatorShell>(*this)),
      _group_coordinator(std::make_unique<GroupCoordinator>(*this)),
      _media_coordinator(std::make_unique<MediaCoordinator>(*this)),
      _call_coordinator(std::make_unique<CallCoordinator>(*this)),
      _profile_coordinator(std::make_unique<ProfileCoordinator>(*this))
{
    const auto chatTransport = _gateway.chatTransport();
    const auto chatDispatcher = _gateway.chatMessageDispatcher();

    connect(_gateway.httpMgr().get(), &HttpMgr::sig_login_mod_finish,
            this, &AppController::onLoginHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_reg_mod_finish,
            this, &AppController::onRegisterHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_reset_mod_finish,
            this, &AppController::onResetHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_settings_mod_finish,
            this, &AppController::onSettingsHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_call_mod_finish,
            this, &AppController::onCallHttpFinished);

    connect(chatTransport.get(), &IChatTransport::sig_con_success,
            this, &AppController::onTcpConnectFinished);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_login_failed,
            this, &AppController::onChatLoginFailed);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_swich_chatdlg,
            this, &AppController::onSwitchToChat);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_add_auth_friend,
            this, &AppController::onAddAuthFriend);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_auth_rsp,
            this, &AppController::onAuthRsp);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_text_chat_msg,
            this, &AppController::onTextChatMsg);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_user_search,
            this, &AppController::onUserSearch);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_friend_apply,
            this, &AppController::onFriendApply);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_notify_offline,
            this, &AppController::onNotifyOffline);
    connect(chatTransport.get(), &IChatTransport::sig_connection_closed,
            this, &AppController::onConnectionClosed);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_list_updated,
            this, &AppController::onGroupListUpdated);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_invite,
            this, &AppController::onGroupInvite);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_apply,
            this, &AppController::onGroupApply);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_member_changed,
            this, &AppController::onGroupMemberChanged);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_chat_msg,
            this, &AppController::onGroupChatMsg);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_group_rsp,
            this, &AppController::onGroupRsp);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_relation_bootstrap_updated,
            this, &AppController::onRelationBootstrapUpdated);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_dialog_list_rsp,
            this, &AppController::onDialogListRsp);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_private_history_rsp,
            this, &AppController::onPrivateHistoryRsp);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_private_msg_changed,
            this, &AppController::onPrivateMsgChanged);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_private_read_ack,
            this, &AppController::onPrivateReadAck);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_message_status,
            this, &AppController::onMessageStatus);
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_call_event,
            this, &AppController::onCallEvent);
    connect(&_livekit_bridge, &LivekitBridge::roomJoined,
            this, &AppController::onLivekitRoomJoined);
    connect(&_livekit_bridge, &LivekitBridge::remoteTrackReady,
            this, &AppController::onLivekitRemoteTrackReady);
    connect(&_livekit_bridge, &LivekitBridge::roomDisconnected,
            this, &AppController::onLivekitRoomDisconnected);
    connect(&_livekit_bridge, &LivekitBridge::permissionError,
            this, &AppController::onLivekitPermissionError);
    connect(&_livekit_bridge, &LivekitBridge::mediaError,
            this, &AppController::onLivekitMediaError);
    connect(&_livekit_bridge, &LivekitBridge::reconnecting,
            this, &AppController::onLivekitReconnecting);
    connect(&_livekit_bridge, &LivekitBridge::bridgeLog, this, [](const QString &message) {
        qInfo().noquote() << "[livekit]" << message;
    });
    connect(chatDispatcher.get(), &ChatMessageDispatcher::sig_heartbeat_ack,
            this, &AppController::onHeartbeatAck);
    connect(&_apply_request_model, &ApplyRequestModel::unapprovedCountChanged,
            this, &AppController::pendingApplyChanged);

    connect(&_register_countdown_timer, &QTimer::timeout,
            this, &AppController::onRegisterCountdownTimeout);
    connect(&_heartbeat_timer, &QTimer::timeout,
            this, &AppController::onHeartbeatTimeout);
    _chat_login_timeout_timer.setSingleShot(true);
    _chat_login_timeout_timer.setInterval(15000);
    connect(&_chat_login_timeout_timer, &QTimer::timeout, this, [this]() {
        if (_reconnecting_chat && _page == ChatPage) {
            _gateway.chatTransport()->CloseConnection();
            return;
        }
        if (!_busy || _page != LoginPage) {
            return;
        }
        _gateway.chatTransport()->CloseConnection();
        setBusy(false);
        setTip("聊天服务登录超时，请重试", true);
    });
}

AppController::Page AppController::page() const
{
    return _page;
}

AppController::~AppController() = default;

CallSessionModel *AppController::callSession()
{
    return &_call_session_model;
}

LivekitBridge *AppController::livekitBridge()
{
    return &_livekit_bridge;
}

void AppController::switchToLogin()
{
    qInfo() << "Switching to login page, current page:" << _page
            << "pending uid:" << _pending_uid
            << "chat connected:" << _gateway.chatTransport()->isConnected();
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    _livekit_bridge.leaveRoom();
    _call_session_model.clear();
    _chat_server_host.clear();
    _chat_server_port.clear();
    _chat_server_name.clear();
    _chat_endpoints.clear();
    _chat_endpoint_index = -1;
    _pending_login_ticket.clear();
    resetReconnectState();
    resetHeartbeatTracking();
    _chat_login_timeout_timer.stop();
    _ignore_next_login_disconnect = true;
    setPage(LoginPage);
    _gateway.chatTransport()->CloseConnection();
    _private_cache_store.close();
    _group_cache_store.close();
    _gateway.userMgr()->ResetSession();
    if (_register_success_page) {
        _register_success_page = false;
        emit registerSuccessPageChanged();
    }
    setBusy(false);
    setTip("", false);
    _chat_list_model.clear();
    _group_list_model.clear();
    _dialog_list_model.clear();
    _contact_list_model.clear();
    _message_model.clear();
    _search_result_model.clear();
    _apply_request_model.clear();
    setSearchPending(false);
    setSearchStatus("", false);
    setChatLoadingMore(false);
    setPrivateHistoryLoading(false);
    _private_history_before_ts = 0;
    _private_history_before_msg_id.clear();
    _private_history_pending_before_ts = 0;
    _private_history_pending_before_msg_id.clear();
    _private_history_pending_peer_uid = 0;
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    _dialog_bootstrap_loading = false;
    _chat_list_initialized = false;
    setDialogsReady(false);
    setContactsReady(false);
    setGroupsReady(false);
    setApplyReady(false);
    _can_load_more_private_history = false;
    emit canLoadMorePrivateHistoryChanged();
    setContactLoadingMore(false);
    setAuthStatus("", false);
    setSettingsStatus("", false);
    setGroupStatus("", false);
    setContactPane(ApplyRequestPane);
    _can_load_more_chats = false;
    emit canLoadMoreChatsChanged();
    _can_load_more_contacts = false;
    emit canLoadMoreContactsChanged();
    setCurrentContact(0, "", "", "qrc:/res/head_1.jpg", "", 0);
    _current_chat_uid = 0;
    _current_group_id = 0;
    _current_group_name.clear();
    _current_group_code.clear();
    emit currentGroupChanged();
    emitCurrentDialogUidChangedIfNeeded();
    _group_uid_map.clear();
    _pending_group_msg_group_map.clear();
    _dialog_draft_map.clear();
    _dialog_pending_attachment_map.clear();
    _dialog_local_pinned_set.clear();
    _dialog_server_pinned_map.clear();
    _dialog_server_mute_map.clear();
    _dialog_mention_map.clear();
    setCurrentDraftText("");
    setCurrentPendingAttachments(QVariantList());
    setCurrentDialogPinned(false);
    setCurrentDialogMuted(false);
    setPendingReplyContext(QString(), QString(), QString());
    setCurrentChatPeerName("");
    setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
    _current_user_id.clear();
    _current_user_desc.clear();
    _pending_uid = 0;
    _pending_token.clear();
    _pending_trace_id.clear();
    _pending_send_queue.clear();
    _pending_send_total_count = 0;
    _pending_send_dialog_uid = 0;
    _pending_send_chat_uid = 0;
    _pending_send_group_id = 0;
    setMediaUploadInProgress(false);
    setMediaUploadProgressText(QString());
    _message_model.setDownloadAuthContext(0, QString());
    setIconDownloadAuthContext(0, QString());
    _settings_avatar_upload_in_progress = false;
    _group_icon_upload_in_progress = false;
}

void AppController::switchToRegister()
{
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    resetHeartbeatTracking();
    if (_register_success_page) {
        _register_success_page = false;
        emit registerSuccessPageChanged();
    }
    setPage(RegisterPage);
    setTip("", false);
}

void AppController::switchToReset()
{
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    resetHeartbeatTracking();
    if (_register_success_page) {
        _register_success_page = false;
        emit registerSuccessPageChanged();
    }
    setPage(ResetPage);
    setTip("", false);
}

void AppController::switchChatTab(int tab)
{
    const int normalized = qBound(0, tab, 2);
    const ChatTab target = static_cast<ChatTab>(normalized);
    if (_chat_tab == target) {
        return;
    }
    _chat_tab = target;
    if (target == ContactTabPage) {
        ensureContactsInitialized();
        ensureApplyInitialized();
    }
    emit chatTabChanged();
}

void AppController::ensureContactsInitialized()
{
    if (_contacts_ready) {
        return;
    }
    bootstrapContacts();
}

void AppController::ensureGroupsInitialized()
{
    if (_groups_ready) {
        return;
    }
    bootstrapGroups();
}

void AppController::ensureApplyInitialized()
{
    if (_apply_ready) {
        return;
    }
    bootstrapApplies();
}

void AppController::ensureChatListInitialized()
{
    if (_chat_list_initialized) {
        return;
    }

    _chat_list_model.clear();
    const auto chatList = _gateway.userMgr()->GetChatListPerPage();
    _chat_list_model.setFriends(chatList);
    _gateway.userMgr()->UpdateChatLoadedCount();
    _chat_list_initialized = true;
    refreshChatLoadMoreState();
}

void AppController::clearTip()
{
    setTip("", false);
}

void AppController::selectChatIndex(int index)
{
    const QVariantMap item = _chat_list_model.get(index);
    if (item.isEmpty()) {
        setPendingReplyContext(QString(), QString(), QString());
        _current_chat_uid = 0;
        setCurrentGroup(0, "");
        setCurrentChatPeerName("");
        setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
        _message_model.clear();
        _private_history_before_ts = 0;
        _private_history_before_msg_id.clear();
        _private_history_pending_before_ts = 0;
        _private_history_pending_before_msg_id.clear();
        _private_history_pending_peer_uid = 0;
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        setCurrentDraftText("");
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
        return;
    }

    setPendingReplyContext(QString(), QString(), QString());
    _current_chat_uid = item.value("uid").toInt();
    _dialog_list_model.clearUnread(_current_chat_uid);
    _chat_list_model.clearUnread(_current_chat_uid);
    setCurrentGroup(0, "");
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    qInfo() << "Selecting private chat by index:" << index
            << "uid:" << _current_chat_uid
            << "name:" << item.value("name").toString();
    setCurrentChatPeerName(item.value("name").toString());
    setCurrentChatPeerIcon(item.value("icon").toString());
    emitCurrentDialogUidChangedIfNeeded();
    loadCurrentChatMessages();
    syncCurrentDialogDraft();
}

void AppController::selectContactIndex(int index)
{
    ensureContactsInitialized();
    const QVariantMap item = _contact_list_model.get(index);
    if (item.isEmpty()) {
        return;
    }

    const QString nick = item.value("nick").toString();
    QString back = item.value("back").toString();
    if (back.isEmpty()) {
        back = nick;
    }

    setCurrentContact(item.value("uid").toInt(),
                      item.value("name").toString(),
                      nick,
                      item.value("icon").toString(),
                      back,
                      item.value("sex").toInt(),
                      item.value("userId").toString());
    setContactPane(FriendInfoPane);
    setAuthStatus("", false);
}

void AppController::selectGroupIndex(int index)
{
    ensureGroupsInitialized();
    const QVariantMap item = _group_list_model.get(index);
    if (item.isEmpty()) {
        setPendingReplyContext(QString(), QString(), QString());
        setCurrentGroup(0, "");
        _message_model.clear();
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        setCanLoadMorePrivateHistory(false);
        setCurrentDraftText("");
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
        return;
    }

    const int pseudoUid = item.value("uid").toInt();
    const qint64 groupId = _group_uid_map.value(pseudoUid, 0);
    if (groupId <= 0) {
        setPendingReplyContext(QString(), QString(), QString());
        setCurrentGroup(0, "");
        _message_model.clear();
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        setCanLoadMorePrivateHistory(false);
        setCurrentDraftText("");
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        emitCurrentDialogUidChangedIfNeeded();
        return;
    }

    QString groupCode;
    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (groupInfo) {
        groupCode = groupInfo->_group_code;
    }
    setPendingReplyContext(QString(), QString(), QString());
    setCurrentGroup(groupId, item.value("name").toString(), groupCode);
    _dialog_list_model.clearUnread(pseudoUid);
    _dialog_list_model.clearMention(pseudoUid);
    _dialog_mention_map.remove(pseudoUid);
    _current_chat_uid = 0;
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(true);
    setCurrentChatPeerName(_current_group_name);
    const QString selectedGroupIcon = item.value("icon").toString().trimmed().isEmpty()
        ? QStringLiteral("qrc:/res/chat_icon.png")
        : item.value("icon").toString();
    setCurrentChatPeerIcon(selectedGroupIcon);
    emitCurrentDialogUidChangedIfNeeded();

    groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo) {
        _message_model.clear();
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (selfInfo && _group_cache_store.isReady()) {
        const auto localMessages = _group_cache_store.loadRecentMessages(selfInfo->_uid, groupId, 50);
        for (const auto &one : localMessages) {
            _gateway.userMgr()->UpsertGroupChatMsg(groupId, one);
        }
    }

    groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo) {
        _message_model.clear();
        return;
    }

    _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    for (const auto &one : groupInfo->_chat_msgs) {
        if (!one || one->_group_seq <= 0) {
            continue;
        }
        if (_group_history_before_seq <= 0 || one->_group_seq < _group_history_before_seq) {
            _group_history_before_seq = one->_group_seq;
        }
    }
    qint64 readTs = 0;
    if (!groupInfo->_chat_msgs.empty() && groupInfo->_chat_msgs.back()) {
        readTs = groupInfo->_chat_msgs.back()->_created_at;
    }
    sendGroupReadAck(groupId, readTs);
    loadGroupHistory();
    syncCurrentDialogDraft();
}

void AppController::selectDialogByUid(int uid)
{
    if (uid == 0) {
        return;
    }

    qInfo() << "Selecting dialog by uid:" << uid
            << "current chat uid:" << _current_chat_uid
            << "current group id:" << _current_group_id;

    if (uid > 0) {
        selectChatByUid(uid);
        return;
    }

    const int groupIndex = _group_list_model.indexOfUid(uid);
    if (groupIndex >= 0) {
        selectGroupIndex(groupIndex);
    }
}

void AppController::showApplyRequests()
{
    ensureApplyInitialized();
    setContactPane(ApplyRequestPane);
    setAuthStatus("", false);
}

void AppController::jumpChatWithCurrentContact()
{
    if (_current_contact_uid <= 0) {
        return;
    }

    selectChatByUid(_current_contact_uid);
    switchChatTab(ChatTabPage);
}

void AppController::loadMoreChats()
{
    ensureChatListInitialized();
    if (_chat_loading_more) {
        return;
    }

    if (_gateway.userMgr()->IsLoadChatFin()) {
        refreshChatLoadMoreState();
        return;
    }

    setChatLoadingMore(true);
    const auto chatList = _gateway.userMgr()->GetChatListPerPage();
    _chat_list_model.appendFriends(chatList);
    _gateway.userMgr()->UpdateChatLoadedCount();
    setChatLoadingMore(false);
    refreshChatLoadMoreState();
}

void AppController::loadMorePrivateHistory()
{
    if (_private_history_loading || !_can_load_more_private_history) {
        return;
    }
    if (_current_group_id > 0) {
        loadGroupHistory();
        return;
    }
    if (_current_chat_uid > 0) {
        qint64 beforeTs = _message_model.earliestCreatedAt();
        QString beforeMsgId = _message_model.earliestMsgId();
        if (beforeTs <= 0) {
            beforeTs = _private_history_before_ts;
            beforeMsgId = _private_history_before_msg_id;
        }
        requestPrivateHistory(_current_chat_uid, beforeTs, beforeMsgId);
    }
}

void AppController::loadMoreContacts()
{
    ensureContactsInitialized();
    if (_contact_loading_more) {
        return;
    }

    if (_gateway.userMgr()->IsLoadConFin()) {
        refreshContactLoadMoreState();
        return;
    }

    setContactLoadingMore(true);
    const auto contactList = _gateway.userMgr()->GetConListPerPage();
    _contact_list_model.appendFriends(contactList);
    _gateway.userMgr()->UpdateContactLoadedCount();
    setContactLoadingMore(false);
    refreshContactLoadMoreState();
}

void AppController::openExternalResource(const QString &url)
{
    QString errorText;
    if (!LocalFilePickerService::openUrl(url, &errorText)) {
        if (errorText.isEmpty()) {
            errorText = "打开资源失败";
        }
        setTip(errorText, true);
    }
}

void AppController::searchUser(const QString &uidText)
{
    _contact_coordinator_shell->searchUser(uidText);
}

void AppController::clearSearchState()
{
    clearSearchResultOnly();
    setSearchPending(false);
    setSearchStatus("", false);
}

void AppController::requestAddFriend(int uid, const QString &bakName, const QVariantList &labels)
{
    _contact_coordinator_shell->requestAddFriend(uid, bakName, labels);
}

void AppController::approveFriend(int uid, const QString &backName, const QVariantList &labels)
{
    _contact_coordinator_shell->approveFriend(uid, backName, labels);
}

void AppController::clearAuthStatus()
{
    setAuthStatus("", false);
}

void AppController::startVoiceChat()
{
    _call_coordinator->startVoiceChat();
}

void AppController::startVideoChat()
{
    _call_coordinator->startVideoChat();
}

void AppController::acceptIncomingCall()
{
    _call_coordinator->acceptIncomingCall();
}

void AppController::rejectIncomingCall()
{
    _call_coordinator->rejectIncomingCall();
}

void AppController::endCurrentCall()
{
    _call_coordinator->endCurrentCall();
}

void AppController::toggleCallMuted()
{
    _call_coordinator->toggleCallMuted();
}

void AppController::toggleCallCamera()
{
    _call_coordinator->toggleCallCamera();
}

void AppController::chooseAvatar()
{
    _profile_coordinator->chooseAvatar();
}

void AppController::saveProfile(const QString &nick, const QString &desc)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setSettingsStatus("用户状态异常，请重新登录", true);
        return;
    }

    QString errorText;
    if (!_profile_controller.validateProfile(nick, desc, &errorText)) {
        setSettingsStatus(errorText, true);
        return;
    }

    const int selfUid = selfInfo->_uid;
    const QString selfName = selfInfo->_name;
    QString iconForSave = _current_user_icon;
    const QUrl iconUrl(iconForSave);
    if (iconUrl.isLocalFile() || QFileInfo(iconForSave).isAbsolute()) {
        if (_settings_avatar_upload_in_progress) {
            setSettingsStatus("头像上传中，请稍候", false);
            return;
        }
        if (_pending_token.trimmed().isEmpty()) {
            setSettingsStatus("登录态失效，请重新登录", true);
            return;
        }

        _settings_avatar_upload_in_progress = true;
        setSettingsStatus("头像上传中...", false);
        const QString uploadPath = iconForSave;
        const QString uploadToken = _pending_token;

        auto *watcher = new QFutureWatcher<MediaUploadTaskResult>(this);
        connect(watcher, &QFutureWatcher<MediaUploadTaskResult>::finished, this,
                [this, watcher, selfUid, selfName, nick, desc]() {
            const MediaUploadTaskResult result = watcher->future().result();
            watcher->deleteLater();
            _settings_avatar_upload_in_progress = false;
            if (!result.ok || result.uploaded.remoteUrl.isEmpty()) {
                setSettingsStatus(result.errorText.isEmpty() ? "头像上传失败" : result.errorText, true);
                return;
            }
            _profile_controller.sendSaveProfile(selfUid, selfName, nick, desc, result.uploaded.remoteUrl);
            setSettingsStatus("资料同步中...", false);
        });

        const auto future = QtConcurrent::run([uploadPath, selfUid, uploadToken]() {
            MediaUploadTaskResult result;
            QString uploadErr;
            result.ok = MediaUploadService::uploadLocalFile(
                uploadPath,
                QStringLiteral("avatar"),
                selfUid,
                uploadToken,
                &result.uploaded,
                &uploadErr);
            if (!result.ok) {
                result.errorText = uploadErr;
            }
            return result;
        });
        watcher->setFuture(future);
        return;
    }

    _profile_controller.sendSaveProfile(
        selfUid, selfName, nick, desc, iconForSave);
    setSettingsStatus("资料同步中...", false);
}

void AppController::clearSettingsStatus()
{
    setSettingsStatus("", false);
}

void AppController::refreshGroupList()
{
    if (!_groups_ready) {
        bootstrapGroups();
        return;
    }
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setGroupStatus("用户状态异常，请重新登录", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GET_GROUP_LIST_REQ, payload);
}

void AppController::requestDialogList()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GET_DIALOG_LIST_REQ, payload);
}

void AppController::createGroup(const QString &name, const QVariantList &memberUserIdList)
{
    _group_coordinator->createGroup(name, memberUserIdList);
}

void AppController::inviteGroupMember(const QString &userId, const QString &reason)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedUserId = userId.trimmed();
    if (!selfInfo || _current_group_id <= 0 || !kUserIdPattern.match(trimmedUserId).hasMatch()) {
        setGroupStatus("邀请参数非法", true);
        return;
    }
    if (selfInfo->_user_id == trimmedUserId) {
        setGroupStatus("不能邀请自己", true);
        return;
    }

    int targetUid = 0;
    const auto friends = _gateway.userMgr()->GetFriendListSnapshot();
    for (const auto &one : friends) {
        if (one && one->_user_id == trimmedUserId) {
            targetUid = one->_uid;
            break;
        }
    }
    if (targetUid <= 0 || !_gateway.userMgr()->CheckFriendById(targetUid)) {
        setGroupStatus("仅支持邀请好友入群", true);
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["target_user_id"] = trimmedUserId;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["reason"] = reason;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_INVITE_GROUP_MEMBER_REQ, payload);
    setGroupStatus("邀请已发送", false);
}

void AppController::applyJoinGroup(const QString &groupCode, const QString &reason)
{
    static const QRegularExpression kGroupCodePattern("^g[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedGroupCode = groupCode.trimmed();
    if (!selfInfo || !kGroupCodePattern.match(trimmedGroupCode).hasMatch()) {
        setGroupStatus("申请参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["group_code"] = trimmedGroupCode;
    obj["reason"] = reason;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_APPLY_JOIN_GROUP_REQ, payload);
    setGroupStatus("申请已发送", false);
}

void AppController::reviewGroupApply(qint64 applyId, bool agree)
{
    _group_coordinator->reviewGroupApply(applyId, agree);
}

void AppController::sendGroupTextMessage(const QString &text)
{
    _group_coordinator->sendGroupTextMessage(text);
}

void AppController::sendGroupImageMessage()
{
    _group_coordinator->sendGroupImageMessage();
}

void AppController::sendGroupFileMessage()
{
    _group_coordinator->sendGroupFileMessage();
}

void AppController::editGroupMessage(const QString &msgId, const QString &text)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedMsgId = msgId.trimmed();
    const QString newText = text.trimmed();
    if (!selfInfo || trimmedMsgId.isEmpty() || newText.isEmpty()) {
        if (_current_group_id > 0) {
            setGroupStatus("编辑消息参数非法", true);
        } else {
            setTip("编辑消息参数非法", true);
        }
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["msgid"] = trimmedMsgId;
    obj["content"] = newText.left(4096);
    if (_current_group_id > 0) {
        obj["groupid"] = static_cast<qint64>(_current_group_id);
    } else if (_current_chat_uid > 0) {
        obj["peer_uid"] = _current_chat_uid;
    } else {
        setTip("请选择会话", true);
        return;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(_current_group_id > 0
                                          ? ReqId::ID_EDIT_GROUP_MSG_REQ
                                          : ReqId::ID_EDIT_PRIVATE_MSG_REQ,
                                      payload);
}

void AppController::revokeGroupMessage(const QString &msgId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedMsgId = msgId.trimmed();
    if (!selfInfo || trimmedMsgId.isEmpty()) {
        if (_current_group_id > 0) {
            setGroupStatus("撤回消息参数非法", true);
        } else {
            setTip("撤回消息参数非法", true);
        }
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["msgid"] = trimmedMsgId;
    if (_current_group_id > 0) {
        obj["groupid"] = static_cast<qint64>(_current_group_id);
    } else if (_current_chat_uid > 0) {
        obj["peer_uid"] = _current_chat_uid;
    } else {
        setTip("请选择会话", true);
        return;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(_current_group_id > 0
                                          ? ReqId::ID_REVOKE_GROUP_MSG_REQ
                                          : ReqId::ID_REVOKE_PRIVATE_MSG_REQ,
                                      payload);
}

void AppController::forwardGroupMessage(const QString &msgId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedMsgId = msgId.trimmed();
    if (!selfInfo || trimmedMsgId.isEmpty()) {
        if (_current_group_id > 0) {
            setGroupStatus("转发消息参数非法", true);
        } else {
            setTip("转发消息参数非法", true);
        }
        return;
    }

    if (_current_group_id <= 0 && _current_chat_uid > 0) {
        if (!_message_model.containsMessage(trimmedMsgId)) {
            setTip("未找到要转发的消息", true);
            return;
        }
        QJsonObject obj;
        obj["fromuid"] = selfInfo->_uid;
        obj["peer_uid"] = _current_chat_uid;
        obj["msgid"] = trimmedMsgId;
        obj["client_msg_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        _gateway.chatTransport()->slot_send_data(ReqId::ID_FORWARD_PRIVATE_MSG_REQ, payload);
        return;
    }
    if (_current_group_id <= 0) {
        setTip("请选择会话", true);
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["msgid"] = trimmedMsgId;
    obj["client_msg_id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_FORWARD_GROUP_MSG_REQ, payload);
}

void AppController::loadGroupHistory()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0) {
        return;
    }
    if (_private_history_loading) {
        return;
    }
    if (!_group_history_has_more && _group_history_before_seq > 0) {
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["before_ts"] = 0;
    obj["before_seq"] = static_cast<qint64>(_group_history_before_seq);
    obj["limit"] = 50;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    setPrivateHistoryLoading(true);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_HISTORY_REQ, payload);
}

void AppController::updateGroupAnnouncement(const QString &announcement)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0) {
        setGroupStatus("请选择群聊", true);
        return;
    }
    if (!currentGroupCanChangeInfo()) {
        setGroupStatus("没有修改群资料权限", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["announcement"] = announcement.left(1000);
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_UPDATE_GROUP_ANNOUNCEMENT_REQ, payload);
}

void AppController::updateGroupIcon()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0) {
        setGroupStatus("请选择群聊", true);
        return;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
    if (!groupInfo) {
        setGroupStatus("群聊不存在或已失效", true);
        return;
    }
    if (!currentGroupCanChangeInfo()) {
        setGroupStatus("没有修改群资料权限", true);
        return;
    }
    if (_group_icon_upload_in_progress) {
        setGroupStatus("群头像上传中，请稍候", false);
        return;
    }

    QString avatarUrl;
    QString errorText;
    if (!LocalFilePickerService::pickAvatarUrl(&avatarUrl, &errorText)) {
        if (!errorText.isEmpty()) {
            setGroupStatus(errorText, true);
        }
        return;
    }

    if (_pending_token.trimmed().isEmpty()) {
        setGroupStatus("登录态失效，请重新登录", true);
        return;
    }

    const int selfUid = selfInfo->_uid;
    const qint64 targetGroupId = _current_group_id;
    const QString uploadToken = _pending_token;
    _group_icon_upload_in_progress = true;
    setGroupStatus("群头像上传中...", false);

    auto *watcher = new QFutureWatcher<MediaUploadTaskResult>(this);
    connect(watcher, &QFutureWatcher<MediaUploadTaskResult>::finished, this,
            [this, watcher, selfUid, targetGroupId]() {
        const MediaUploadTaskResult result = watcher->future().result();
        watcher->deleteLater();
        _group_icon_upload_in_progress = false;
        if (!result.ok || result.uploaded.remoteUrl.isEmpty()) {
            setGroupStatus(result.errorText.isEmpty() ? "群头像上传失败" : result.errorText, true);
            return;
        }

        QJsonObject obj;
        obj["fromuid"] = selfUid;
        obj["groupid"] = static_cast<qint64>(targetGroupId);
        obj["icon"] = result.uploaded.remoteUrl;
        const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        _gateway.chatTransport()->slot_send_data(ReqId::ID_UPDATE_GROUP_ICON_REQ, payload);
        setGroupStatus("群头像更新中...", false);
    });

    const auto future = QtConcurrent::run([avatarUrl, selfUid, uploadToken]() {
        MediaUploadTaskResult result;
        QString uploadErr;
        result.ok = MediaUploadService::uploadLocalFile(
            avatarUrl,
            QStringLiteral("group_avatar"),
            selfUid,
            uploadToken,
            &result.uploaded,
            &uploadErr);
        if (!result.ok) {
            result.errorText = uploadErr;
        }
        return result;
    });
    watcher->setFuture(future);
}

void AppController::setGroupAdmin(const QString &userId, bool isAdmin, qint64 permissionBits)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedUserId = userId.trimmed();
    if (!selfInfo || _current_group_id <= 0 || !kUserIdPattern.match(trimmedUserId).hasMatch()) {
        setGroupStatus("设置管理员参数非法", true);
        return;
    }
    if (!currentGroupCanManageAdmins()) {
        setGroupStatus("没有管理管理员权限", true);
        return;
    }
    qint64 normalizedPermBits = permissionBits;
    if (isAdmin) {
        if (normalizedPermBits <= 0) {
            normalizedPermBits = kDefaultAdminPermBits;
        }
        normalizedPermBits &= kOwnerPermBits;
        if (normalizedPermBits <= 0) {
            normalizedPermBits = kDefaultAdminPermBits;
        }
    } else {
        normalizedPermBits = 0;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["target_user_id"] = trimmedUserId;
    obj["is_admin"] = isAdmin;
    obj["permission_bits"] = normalizedPermBits;
    obj["can_change_group_info"] = (normalizedPermBits & kPermChangeGroupInfo) != 0;
    obj["can_delete_messages"] = (normalizedPermBits & kPermDeleteMessages) != 0;
    obj["can_invite_users"] = (normalizedPermBits & kPermInviteUsers) != 0;
    obj["can_manage_admins"] = (normalizedPermBits & kPermManageAdmins) != 0;
    obj["can_pin_messages"] = (normalizedPermBits & kPermPinMessages) != 0;
    obj["can_ban_users"] = (normalizedPermBits & kPermBanUsers) != 0;
    obj["can_manage_topics"] = (normalizedPermBits & kPermManageTopics) != 0;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_SET_GROUP_ADMIN_REQ, payload);
}

void AppController::muteGroupMember(const QString &userId, int muteSeconds)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedUserId = userId.trimmed();
    if (!selfInfo || _current_group_id <= 0 || !kUserIdPattern.match(trimmedUserId).hasMatch()) {
        setGroupStatus("禁言参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["target_user_id"] = trimmedUserId;
    obj["mute_seconds"] = qMax(0, muteSeconds);
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_MUTE_GROUP_MEMBER_REQ, payload);
}

void AppController::kickGroupMember(const QString &userId)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedUserId = userId.trimmed();
    if (!selfInfo || _current_group_id <= 0 || !kUserIdPattern.match(trimmedUserId).hasMatch()) {
        setGroupStatus("踢人参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["target_user_id"] = trimmedUserId;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_KICK_GROUP_MEMBER_REQ, payload);
}

void AppController::quitCurrentGroup()
{
    _group_coordinator->quitCurrentGroup();
}

void AppController::clearGroupStatus()
{
    setGroupStatus("", false);
}

void AppController::updateCurrentDraft(const QString &text)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const int dialogUid = currentDialogUid();
    if (!selfInfo || dialogUid == 0) {
        return;
    }

    QString nextDraft = text;
    if (nextDraft.size() > 2000) {
        nextDraft = nextDraft.left(2000);
    }
    const QString normalizedDraft = nextDraft.trimmed().isEmpty() ? QString() : nextDraft;
    if (normalizedDraft.isEmpty()) {
        _dialog_draft_map.remove(dialogUid);
    } else {
        _dialog_draft_map.insert(dialogUid, normalizedDraft);
    }

    setCurrentDraftText(normalizedDraft);
    applyDraftToDialogModel(dialogUid, normalizedDraft);
    const int ownerUid = _gateway.userMgr()->GetUid();
    if (ownerUid > 0) {
        saveDraftStore(ownerUid);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    if (_current_group_id > 0) {
        obj["dialog_type"] = "group";
        obj["groupid"] = static_cast<qint64>(_current_group_id);
    } else if (_current_chat_uid > 0) {
        obj["dialog_type"] = "private";
        obj["peer_uid"] = _current_chat_uid;
    } else {
        return;
    }
    obj["draft_text"] = normalizedDraft;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_SYNC_DRAFT_REQ, payload);
}

void AppController::toggleCurrentDialogPinned()
{
    const int dialogUid = currentDialogUid();
    toggleDialogPinnedByUid(dialogUid);
}

void AppController::toggleCurrentDialogMuted()
{
    const int dialogUid = currentDialogUid();
    toggleDialogMutedByUid(dialogUid);
}

void AppController::toggleDialogPinnedByUid(int dialogUid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || dialogUid == 0) {
        return;
    }

    QString dialogType;
    int peerUid = 0;
    qint64 groupId = 0;
    if (!resolveDialogTarget(dialogUid, dialogType, peerUid, groupId)) {
        return;
    }

    const bool currentlyPinned = _dialog_local_pinned_set.contains(dialogUid)
        || _dialog_server_pinned_map.value(dialogUid, 0) > 0;
    const bool nextPinned = !currentlyPinned;
    if (!nextPinned) {
        _dialog_local_pinned_set.remove(dialogUid);
        _dialog_server_pinned_map.insert(dialogUid, 0);
    } else {
        _dialog_local_pinned_set.insert(dialogUid);
        _dialog_server_pinned_map.insert(dialogUid,
                                         static_cast<int>(QDateTime::currentSecsSinceEpoch()));
    }
    if (dialogUid == currentDialogUid()) {
        setCurrentDialogPinned(nextPinned);
    }

    const int ownerUid = _gateway.userMgr()->GetUid();
    if (ownerUid > 0) {
        saveDraftStore(ownerUid);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["dialog_type"] = dialogType;
    if (dialogType == "group") {
        obj["groupid"] = static_cast<qint64>(groupId);
    } else {
        obj["peer_uid"] = peerUid;
    }
    obj["pinned_rank"] = nextPinned ? static_cast<int>(QDateTime::currentSecsSinceEpoch()) : 0;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_PIN_DIALOG_REQ, payload);

    refreshDialogModel();
}

void AppController::toggleDialogMutedByUid(int dialogUid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || dialogUid == 0) {
        return;
    }

    QString dialogType;
    int peerUid = 0;
    qint64 groupId = 0;
    if (!resolveDialogTarget(dialogUid, dialogType, peerUid, groupId)) {
        return;
    }

    const bool currentlyMuted = _dialog_server_mute_map.value(dialogUid, 0) > 0;
    const int nextMuteState = currentlyMuted ? 0 : 1;
    _dialog_server_mute_map.insert(dialogUid, nextMuteState);
    if (dialogUid == currentDialogUid()) {
        setCurrentDialogMuted(nextMuteState > 0);
    }

    const int idx = _dialog_list_model.indexOfUid(dialogUid);
    if (idx >= 0) {
        const QVariantMap item = _dialog_list_model.get(idx);
        _dialog_list_model.setDialogMeta(dialogUid,
                                         item.value("dialogType").toString(),
                                         item.value("unreadCount").toInt(),
                                         item.value("pinnedRank").toInt(),
                                         item.value("draftText").toString(),
                                         item.value("lastMsgTs").toLongLong(),
                                         nextMuteState);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["dialog_type"] = dialogType;
    if (dialogType == "group") {
        obj["groupid"] = static_cast<qint64>(groupId);
    } else {
        obj["peer_uid"] = peerUid;
    }
    obj["draft_text"] = _dialog_draft_map.value(dialogUid);
    obj["mute_state"] = nextMuteState;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_SYNC_DRAFT_REQ, payload);
}

void AppController::markDialogReadByUid(int dialogUid)
{
    if (dialogUid == 0) {
        return;
    }

    _dialog_list_model.clearUnread(dialogUid);
    _dialog_list_model.clearMention(dialogUid);
    _dialog_mention_map.remove(dialogUid);
    if (dialogUid > 0) {
        _chat_list_model.clearUnread(dialogUid);
        const qint64 latestPeerTs = latestPrivatePeerCreatedAt(dialogUid);
        if (latestPeerTs > 0) {
            sendPrivateReadAck(dialogUid, latestPeerTs);
        } else {
            sendPrivateReadAck(dialogUid, 0);
        }
        return;
    }

    _group_list_model.clearUnread(dialogUid);
    const qint64 groupId = -static_cast<qint64>(dialogUid);
    const qint64 latestGroupTs = latestGroupCreatedAt(groupId);
    if (latestGroupTs > 0) {
        sendGroupReadAck(groupId, latestGroupTs);
    } else {
        sendGroupReadAck(groupId, 0);
    }
}

void AppController::clearDialogDraftByUid(int dialogUid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || dialogUid == 0) {
        return;
    }

    QString dialogType;
    int peerUid = 0;
    qint64 groupId = 0;
    if (!resolveDialogTarget(dialogUid, dialogType, peerUid, groupId)) {
        return;
    }

    _dialog_draft_map.remove(dialogUid);
    applyDraftToDialogModel(dialogUid, QString());
    if (dialogUid == currentDialogUid()) {
        setCurrentDraftText(QString());
    }
    const int ownerUid = _gateway.userMgr()->GetUid();
    if (ownerUid > 0) {
        saveDraftStore(ownerUid);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["dialog_type"] = dialogType;
    if (dialogType == "group") {
        obj["groupid"] = static_cast<qint64>(groupId);
    } else {
        obj["peer_uid"] = peerUid;
    }
    obj["draft_text"] = "";
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_SYNC_DRAFT_REQ, payload);
}

void AppController::beginReplyMessage(const QString &msgId, const QString &senderName, const QString &previewText)
{
    const QString normalizedMsgId = msgId.trimmed();
    if (normalizedMsgId.isEmpty()) {
        return;
    }
    QString normalizedPreview = previewText.trimmed();
    if (normalizedPreview.length() > 120) {
        normalizedPreview = normalizedPreview.left(120);
    }
    setPendingReplyContext(normalizedMsgId, senderName.trimmed(), normalizedPreview);
}

void AppController::cancelReplyMessage()
{
    setPendingReplyContext(QString(), QString(), QString());
}

void AppController::login(const QString &email, const QString &password)
{
    _session_coordinator->login(email, password);
}

void AppController::requestRegisterCode(const QString &email)
{
    _session_coordinator->requestRegisterCode(email);
}

void AppController::registerUser(const QString &user, const QString &email, const QString &password,
                                 const QString &confirm, const QString &verifyCode)
{
    _session_coordinator->registerUser(user, email, password, confirm, verifyCode);
}

void AppController::requestResetCode(const QString &email)
{
    _session_coordinator->requestResetCode(email);
}

void AppController::resetPassword(const QString &user, const QString &email, const QString &password,
                                  const QString &verifyCode)
{
    _session_coordinator->resetPassword(user, email, password, verifyCode);
}





void AppController::refreshFriendModels()
{
    ensureChatListInitialized();
    bootstrapContacts();
    bootstrapGroups();
    refreshDialogModel();
}

void AppController::refreshApplyModel()
{
    const auto applyList = _gateway.userMgr()->GetApplyListSnapshot();
    _apply_request_model.setApplies(applyList);
}

void AppController::bootstrapDialogs()
{
    if (_dialogs_ready) {
        return;
    }
    _dialog_bootstrap_loading = true;
    setDialogsReady(false);
    requestDialogList();
}

void AppController::bootstrapContacts()
{
    if (_contacts_ready) {
        return;
    }

    ensureChatListInitialized();
    _contact_list_model.clear();
    const auto contactList = _gateway.userMgr()->GetConListPerPage();
    _contact_list_model.setFriends(contactList);
    _gateway.userMgr()->UpdateContactLoadedCount();
    refreshContactLoadMoreState();
    setContactsReady(true);
}

void AppController::bootstrapGroups()
{
    if (_groups_ready) {
        return;
    }

    refreshGroupModel();
    setGroupsReady(true);
    if (auto selfInfo = _gateway.userMgr()->GetUserInfo()) {
        QTimer::singleShot(0, this, [this, uid = selfInfo->_uid]() {
            auto current = _gateway.userMgr()->GetUserInfo();
            if (!current || current->_uid != uid || _page != ChatPage) {
                return;
            }
            refreshGroupList();
        });
    }
}

void AppController::bootstrapApplies()
{
    if (_apply_ready) {
        return;
    }

    refreshApplyModel();
    setApplyReady(true);
}

void AppController::refreshGroupModel()
{
    _group_list_model.clear();
    _group_uid_map.clear();

    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    std::vector<std::shared_ptr<FriendInfo>> converted;
    converted.reserve(groups.size());
    for (const auto &group : groups) {
        if (!group || group->_group_id <= 0) {
            continue;
        }
        const int pseudoUid = -static_cast<int>(group->_group_id);
        const QString groupIcon = group->_icon.trimmed().isEmpty()
            ? QStringLiteral("qrc:/res/chat_icon.png")
            : group->_icon;
        auto info = std::make_shared<FriendInfo>(pseudoUid,
                                                 group->_name,
                                                 group->_name,
                                                 groupIcon,
                                                 0,
                                                 group->_announcement,
                                                 group->_announcement,
                                                 group->_last_msg);
        converted.push_back(info);
        _group_uid_map.insert(pseudoUid, group->_group_id);
    }

    _group_list_model.setFriends(converted);
}

void AppController::refreshDialogModel()
{
    _dialog_list_model.clear();
    std::vector<std::shared_ptr<FriendInfo>> dialogs;
    const DialogDecorationState decorationState {
        &_dialog_draft_map,
        &_dialog_mention_map,
        &_dialog_server_pinned_map,
        &_dialog_server_mute_map,
        &_dialog_local_pinned_set
    };

    const auto chats = _gateway.userMgr()->GetFriendListSnapshot();
    dialogs.reserve(chats.size() + _group_uid_map.size());
    for (const auto &chat : chats) {
        if (!chat) {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = chat->_uid;
        seed.dialogType = QStringLiteral("private");
        seed.userId = chat->_user_id;
        seed.name = chat->_name;
        seed.nick = chat->_nick;
        seed.icon = chat->_icon;
        seed.desc = chat->_desc;
        seed.back = chat->_back;
        seed.previewText = chat->_last_msg;
        seed.sex = chat->_sex;
        if (!chat->_chat_msgs.empty() && chat->_chat_msgs.back()) {
            seed.lastMsgTs = chat->_chat_msgs.back()->_created_at;
        }
        dialogs.push_back(DialogListService::buildDialogEntry(seed, decorationState));
    }

    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    for (const auto &group : groups) {
        if (!group || group->_group_id <= 0) {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = -static_cast<int>(group->_group_id);
        seed.dialogType = QStringLiteral("group");
        seed.name = group->_name;
        seed.nick = group->_name;
        seed.icon = group->_icon.trimmed().isEmpty()
            ? QStringLiteral("qrc:/res/chat_icon.png")
            : group->_icon;
        seed.desc = group->_announcement;
        seed.back = group->_announcement;
        seed.previewText = group->_last_msg;
        if (!group->_chat_msgs.empty() && group->_chat_msgs.back()) {
            seed.lastMsgTs = group->_chat_msgs.back()->_created_at;
        }
        dialogs.push_back(DialogListService::buildDialogEntry(seed, decorationState));
    }

    DialogListService::sortDialogs(dialogs);
    _dialog_list_model.setFriends(dialogs);
    syncCurrentDialogDraft();
}

void AppController::refreshDialogModelIncremental()
{
    const qint64 startTs = QDateTime::currentMSecsSinceEpoch();

    // 增量更新会话列表：避免全量重建，使用 upsert 增量添加
    const DialogDecorationState decorationState {
        &_dialog_draft_map,
        &_dialog_mention_map,
        &_dialog_server_pinned_map,
        &_dialog_server_mute_map,
        &_dialog_local_pinned_set
    };

    // 增量更新私聊会话
    const auto chats = _gateway.userMgr()->GetFriendListSnapshot();
    for (const auto &chat : chats) {
        if (!chat) {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = chat->_uid;
        seed.dialogType = QStringLiteral("private");
        seed.userId = chat->_user_id;
        seed.name = chat->_name;
        seed.nick = chat->_nick;
        seed.icon = chat->_icon;
        seed.desc = chat->_desc;
        seed.back = chat->_back;
        seed.previewText = chat->_last_msg;
        seed.sex = chat->_sex;
        if (!chat->_chat_msgs.empty() && chat->_chat_msgs.back()) {
            seed.lastMsgTs = chat->_chat_msgs.back()->_created_at;
        }
        auto entry = DialogListService::buildDialogEntry(seed, decorationState);
        _dialog_list_model.upsertFriend(entry);
    }

    // 增量更新群组会话
    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    for (const auto &group : groups) {
        if (!group || group->_group_id <= 0) {
            continue;
        }

        DialogEntrySeed seed;
        seed.dialogUid = -static_cast<int>(group->_group_id);
        seed.dialogType = QStringLiteral("group");
        seed.name = group->_name;
        seed.nick = group->_name;
        seed.icon = group->_icon.trimmed().isEmpty()
            ? QStringLiteral("qrc:/res/chat_icon.png")
            : group->_icon;
        seed.desc = group->_announcement;
        seed.back = group->_announcement;
        seed.previewText = group->_last_msg;
        if (!group->_chat_msgs.empty() && group->_chat_msgs.back()) {
            seed.lastMsgTs = group->_chat_msgs.back()->_created_at;
        }
        auto entry = DialogListService::buildDialogEntry(seed, decorationState);
        _dialog_list_model.upsertFriend(entry);
    }

    syncCurrentDialogDraft();

    const qint64 endTs = QDateTime::currentMSecsSinceEpoch();
    qInfo() << "[PERF] refreshDialogModelIncremental - chats:" << chats.size()
            << "| groups:" << groups.size()
            << "| total:" << (endTs - startTs) << "ms";
}

void AppController::loadCurrentChatMessages()
{
    if (_current_chat_uid <= 0) {
        _message_model.clear();
        _private_history_before_ts = 0;
        _private_history_before_msg_id.clear();
        _private_history_pending_before_ts = 0;
        _private_history_pending_before_msg_id.clear();
        _private_history_pending_peer_uid = 0;
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!friendInfo || !selfInfo) {
        _message_model.clear();
        _private_history_before_ts = 0;
        _private_history_before_msg_id.clear();
        _private_history_pending_before_ts = 0;
        _private_history_pending_before_msg_id.clear();
        _private_history_pending_peer_uid = 0;
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        return;
    }

    qInfo() << "Loading current private chat, peer uid:" << _current_chat_uid
            << "existing friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size());
    setCurrentChatPeerName(friendInfo->_name);
    setCurrentChatPeerIcon(friendInfo->_icon);

    const auto localMessages = _private_cache_store.loadRecentMessages(selfInfo->_uid, _current_chat_uid, 20);
    if (!localMessages.empty()) {
        _gateway.userMgr()->AppendFriendChatMsg(_current_chat_uid, localMessages);
        qInfo() << "Merged local private cache, peer uid:" << _current_chat_uid
                << "cache count:" << static_cast<qlonglong>(localMessages.size());
    }
    friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
    if (!friendInfo) {
        _message_model.clear();
        setCanLoadMorePrivateHistory(false);
        return;
    }
    _message_model.setMessages(friendInfo->_chat_msgs, selfInfo->_uid);
    qint64 latestPeerTs = 0;
    for (const auto &one : friendInfo->_chat_msgs) {
        if (one && one->_from_uid == _current_chat_uid) {
            latestPeerTs = qMax(latestPeerTs, one->_created_at);
        }
    }
    if (latestPeerTs > 0) {
        sendPrivateReadAck(_current_chat_uid, latestPeerTs);
    }
    _private_history_before_ts = _message_model.earliestCreatedAt();
    _private_history_before_msg_id = _message_model.earliestMsgId();
    _private_history_pending_before_ts = 0;
    _private_history_pending_before_msg_id.clear();
    _private_history_pending_peer_uid = 0;
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(true);
    qInfo() << "Private chat loaded, peer uid:" << _current_chat_uid
            << "friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size())
            << "message model count:" << _message_model.rowCount()
            << "earliest created at:" << _private_history_before_ts
            << "earliest msg id:" << _private_history_before_msg_id;
    requestPrivateHistory(_current_chat_uid, _private_history_before_ts, _private_history_before_msg_id);
}

void AppController::requestPrivateHistory(int peerUid, qint64 beforeTs, const QString &beforeMsgId)
{
    if (_private_history_loading || peerUid <= 0) {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }

    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["peer_uid"] = peerUid;
    payloadObj["before_ts"] = static_cast<qint64>(beforeTs);
    if (!beforeMsgId.trimmed().isEmpty()) {
        payloadObj["before_msg_id"] = beforeMsgId.trimmed();
    }
    payloadObj["limit"] = 20;
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);

    _private_history_pending_peer_uid = peerUid;
    _private_history_pending_before_ts = beforeTs;
    _private_history_pending_before_msg_id = beforeMsgId.trimmed();
    setPrivateHistoryLoading(true);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_PRIVATE_HISTORY_REQ, payload);
}

void AppController::sendGroupReadAck(qint64 groupId, qint64 readTs)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || groupId <= 0) {
        return;
    }
    if (readTs <= 0) {
        readTs = QDateTime::currentMSecsSinceEpoch();
    }
    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["groupid"] = static_cast<qint64>(groupId);
    payloadObj["read_ts"] = static_cast<qint64>(readTs);
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_READ_ACK_REQ, payload);
}

void AppController::sendPrivateReadAck(int peerUid, qint64 readTs)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || peerUid <= 0) {
        return;
    }
    if (readTs <= 0) {
        readTs = QDateTime::currentMSecsSinceEpoch();
    }
    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["peer_uid"] = peerUid;
    payloadObj["read_ts"] = static_cast<qint64>(readTs);
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_PRIVATE_READ_ACK_REQ, payload);
}

void AppController::selectChatByUid(int uid)
{
    qInfo() << "Selecting private chat by uid:" << uid
            << "current chat uid:" << _current_chat_uid
            << "current group id:" << _current_group_id;
    const int idx = _chat_list_model.indexOfUid(uid);
    if (idx >= 0) {
        selectChatIndex(idx);
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(uid);
    if (!friendInfo) {
        const int dialogIndex = _dialog_list_model.indexOfUid(uid);
        const QVariantMap dialogItem = dialogIndex >= 0 ? _dialog_list_model.get(dialogIndex) : QVariantMap();
        auto placeholder = DialogListService::buildPlaceholderAuthInfo(
            uid,
            dialogItem,
            QStringLiteral("qrc:/res/head_1.jpg"));
        _gateway.userMgr()->AddFriend(placeholder);
        _chat_list_model.upsertFriend(placeholder);
        _contact_list_model.upsertFriend(placeholder);
        _dialog_list_model.upsertFriend(placeholder);
        friendInfo = _gateway.userMgr()->GetFriendById(uid);
        if (!friendInfo) {
            return;
        }
    }

    _current_chat_uid = uid;
    _dialog_list_model.clearUnread(uid);
    _chat_list_model.clearUnread(uid);
    setCurrentGroup(0, "");
    setCurrentChatPeerName(friendInfo->_name);
    setCurrentChatPeerIcon(friendInfo->_icon);
    emitCurrentDialogUidChangedIfNeeded();
    qInfo() << "Private chat resolved, uid:" << uid
            << "name:" << friendInfo->_name
            << "friend msg count:" << static_cast<qlonglong>(friendInfo->_chat_msgs.size());
    loadCurrentChatMessages();
    syncCurrentDialogDraft();
}

bool AppController::parseJson(const QString &res, QJsonObject &obj)
{
    return _auth_controller.parseJson(res, obj);
}

bool AppController::checkEmailValid(const QString &email)
{
    QString errorText;
    if (!_auth_controller.checkEmail(email, &errorText)) {
        setTip(errorText, true);
        return false;
    }
    return true;
}

bool AppController::checkPwdValid(const QString &password)
{
    QString errorText;
    if (!_auth_controller.checkPassword(password, &errorText)) {
        setTip(errorText, true);
        return false;
    }
    return true;
}

bool AppController::checkUserValid(const QString &user)
{
    QString errorText;
    if (!_auth_controller.checkUser(user, &errorText)) {
        setTip(errorText, true);
        return false;
    }
    return true;
}

bool AppController::checkVerifyCodeValid(const QString &code)
{
    QString errorText;
    if (!_auth_controller.checkVerifyCode(code, &errorText)) {
        setTip(errorText, true);
        return false;
    }
    return true;
}



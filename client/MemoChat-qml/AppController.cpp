#include "AppController.h"
#include "LocalFilePickerService.h"
#include "MediaUploadService.h"
#include "MessageContentCodec.h"
#include "IconPathUtils.h"
#include "httpmgr.h"
#include "tcpmgr.h"
#include "usermgr.h"
#include <QDateTime>
#include <algorithm>
#include <QFileInfo>
#include <QUuid>
#include <QRegularExpression>
#include <QUrl>
#include <QSettings>
#include <QVariantMap>

namespace {
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
      _private_history_pending_before_ts(0),
      _private_history_pending_peer_uid(0),
      _group_history_before_seq(0),
      _group_history_has_more(true),
      _auth_controller(&_gateway),
      _chat_controller(&_gateway),
      _contact_controller(&_gateway),
      _profile_controller(&_gateway)
{
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_login_mod_finish,
            this, &AppController::onLoginHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_reg_mod_finish,
            this, &AppController::onRegisterHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_reset_mod_finish,
            this, &AppController::onResetHttpFinished);
    connect(_gateway.httpMgr().get(), &HttpMgr::sig_settings_mod_finish,
            this, &AppController::onSettingsHttpFinished);

    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_con_success,
            this, &AppController::onTcpConnectFinished);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_login_failed,
            this, &AppController::onChatLoginFailed);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_swich_chatdlg,
            this, &AppController::onSwitchToChat);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_add_auth_friend,
            this, &AppController::onAddAuthFriend);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_auth_rsp,
            this, &AppController::onAuthRsp);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_text_chat_msg,
            this, &AppController::onTextChatMsg);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_user_search,
            this, &AppController::onUserSearch);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_friend_apply,
            this, &AppController::onFriendApply);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_notify_offline,
            this, &AppController::onNotifyOffline);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_connection_closed,
            this, &AppController::onConnectionClosed);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_group_list_updated,
            this, &AppController::onGroupListUpdated);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_group_invite,
            this, &AppController::onGroupInvite);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_group_apply,
            this, &AppController::onGroupApply);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_group_member_changed,
            this, &AppController::onGroupMemberChanged);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_group_chat_msg,
            this, &AppController::onGroupChatMsg);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_group_rsp,
            this, &AppController::onGroupRsp);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_dialog_list_rsp,
            this, &AppController::onDialogListRsp);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_private_history_rsp,
            this, &AppController::onPrivateHistoryRsp);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_private_msg_changed,
            this, &AppController::onPrivateMsgChanged);
    connect(_gateway.tcpMgr().get(), &TcpMgr::sig_private_read_ack,
            this, &AppController::onPrivateReadAck);
    connect(&_apply_request_model, &ApplyRequestModel::unapprovedCountChanged,
            this, &AppController::pendingApplyChanged);

    connect(&_register_countdown_timer, &QTimer::timeout,
            this, &AppController::onRegisterCountdownTimeout);
    connect(&_heartbeat_timer, &QTimer::timeout,
            this, &AppController::onHeartbeatTimeout);
    _chat_login_timeout_timer.setSingleShot(true);
    _chat_login_timeout_timer.setInterval(8000);
    connect(&_chat_login_timeout_timer, &QTimer::timeout, this, [this]() {
        if (!_busy || _page != LoginPage) {
            return;
        }
        _gateway.tcpMgr()->CloseConnection();
        setBusy(false);
        setTip("聊天服务登录超时，请重试", true);
    });
}

AppController::Page AppController::page() const
{
    return _page;
}

QString AppController::tipText() const
{
    return _tip_text;
}

bool AppController::tipError() const
{
    return _tip_error;
}

bool AppController::busy() const
{
    return _busy;
}

bool AppController::registerSuccessPage() const
{
    return _register_success_page;
}

int AppController::registerCountdown() const
{
    return _register_countdown;
}

AppController::ChatTab AppController::chatTab() const
{
    return _chat_tab;
}

AppController::ContactPane AppController::contactPane() const
{
    return _contact_pane;
}

QString AppController::currentUserName() const
{
    return _current_user_name;
}

QString AppController::currentUserNick() const
{
    return _current_user_nick;
}

QString AppController::currentUserIcon() const
{
    return _current_user_icon;
}

QString AppController::currentUserDesc() const
{
    return _current_user_desc;
}

QString AppController::currentUserId() const
{
    return _current_user_id;
}

QString AppController::currentContactName() const
{
    return _current_contact_name;
}

QString AppController::currentContactNick() const
{
    return _current_contact_nick;
}

QString AppController::currentContactIcon() const
{
    return _current_contact_icon;
}

QString AppController::currentContactBack() const
{
    return _current_contact_back;
}

int AppController::currentContactSex() const
{
    return _current_contact_sex;
}

QString AppController::currentContactUserId() const
{
    return _current_contact_user_id;
}

bool AppController::hasCurrentContact() const
{
    return _current_contact_uid > 0;
}

QString AppController::currentChatPeerName() const
{
    return _current_chat_peer_name;
}

QString AppController::currentChatPeerIcon() const
{
    return _current_chat_peer_icon;
}

bool AppController::hasCurrentChat() const
{
    return _current_chat_uid > 0 || _current_group_id > 0;
}

bool AppController::hasCurrentGroup() const
{
    return _current_group_id > 0;
}

int AppController::currentGroupRole() const
{
    if (_current_group_id <= 0) {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
    if (!groupInfo) {
        return 0;
    }
    return groupInfo->_role;
}

QString AppController::currentGroupName() const
{
    return _current_group_name;
}

QString AppController::currentGroupCode() const
{
    return _current_group_code;
}

bool AppController::currentGroupCanChangeInfo() const
{
    return hasCurrentGroupPermission(kPermChangeGroupInfo);
}

bool AppController::currentGroupCanInviteUsers() const
{
    return hasCurrentGroupPermission(kPermInviteUsers);
}

bool AppController::currentGroupCanManageAdmins() const
{
    return hasCurrentGroupPermission(kPermManageAdmins);
}

bool AppController::currentGroupCanBanUsers() const
{
    return hasCurrentGroupPermission(kPermBanUsers);
}

FriendListModel *AppController::dialogListModel()
{
    return &_dialog_list_model;
}

FriendListModel *AppController::chatListModel()
{
    return &_chat_list_model;
}

FriendListModel *AppController::groupListModel()
{
    return &_group_list_model;
}

FriendListModel *AppController::contactListModel()
{
    return &_contact_list_model;
}

ChatMessageModel *AppController::messageModel()
{
    return &_message_model;
}

SearchResultModel *AppController::searchResultModel()
{
    return &_search_result_model;
}

ApplyRequestModel *AppController::applyRequestModel()
{
    return &_apply_request_model;
}

bool AppController::searchPending() const
{
    return _search_pending;
}

QString AppController::searchStatusText() const
{
    return _search_status_text;
}

bool AppController::searchStatusError() const
{
    return _search_status_error;
}

bool AppController::hasPendingApply() const
{
    return _apply_request_model.hasUnapproved();
}

bool AppController::chatLoadingMore() const
{
    return _chat_loading_more;
}

bool AppController::canLoadMoreChats() const
{
    return _can_load_more_chats;
}

bool AppController::privateHistoryLoading() const
{
    return _private_history_loading;
}

bool AppController::canLoadMorePrivateHistory() const
{
    return _can_load_more_private_history;
}

bool AppController::contactLoadingMore() const
{
    return _contact_loading_more;
}

bool AppController::canLoadMoreContacts() const
{
    return _can_load_more_contacts;
}

QString AppController::authStatusText() const
{
    return _auth_status_text;
}

bool AppController::authStatusError() const
{
    return _auth_status_error;
}

QString AppController::settingsStatusText() const
{
    return _settings_status_text;
}

bool AppController::settingsStatusError() const
{
    return _settings_status_error;
}

QString AppController::groupStatusText() const
{
    return _group_status_text;
}

bool AppController::groupStatusError() const
{
    return _group_status_error;
}

QString AppController::currentDraftText() const
{
    return _current_draft_text;
}

bool AppController::currentDialogPinned() const
{
    return _current_dialog_pinned;
}

bool AppController::currentDialogMuted() const
{
    return _current_dialog_muted;
}

bool AppController::hasPendingReply() const
{
    return !_reply_to_msg_id.isEmpty();
}

QString AppController::replyTargetName() const
{
    return _reply_target_name;
}

QString AppController::replyPreviewText() const
{
    return _reply_preview_text;
}

void AppController::switchToLogin()
{
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
    _chat_login_timeout_timer.stop();
    _private_cache_store.close();
    _group_cache_store.close();
    _gateway.userMgr()->ResetSession();
    if (_register_success_page) {
        _register_success_page = false;
        emit registerSuccessPageChanged();
    }
    setBusy(false);
    setPage(LoginPage);
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
    _private_history_pending_before_ts = 0;
    _private_history_pending_peer_uid = 0;
    _group_history_before_seq = 0;
    _group_history_has_more = true;
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
    _group_uid_map.clear();
    _pending_group_msg_group_map.clear();
    _dialog_draft_map.clear();
    _dialog_local_pinned_set.clear();
    _dialog_server_pinned_map.clear();
    _dialog_server_mute_map.clear();
    _dialog_mention_map.clear();
    setCurrentDraftText("");
    setCurrentDialogPinned(false);
    setCurrentDialogMuted(false);
    setPendingReplyContext(QString(), QString(), QString());
    setCurrentChatPeerName("");
    setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
    _current_user_id.clear();
    _current_user_desc.clear();
}

void AppController::switchToRegister()
{
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
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
    emit chatTabChanged();
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
        _private_history_pending_before_ts = 0;
        _private_history_pending_peer_uid = 0;
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        setCurrentDraftText("");
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        return;
    }

    setPendingReplyContext(QString(), QString(), QString());
    _current_chat_uid = item.value("uid").toInt();
    _dialog_list_model.clearUnread(_current_chat_uid);
    setCurrentGroup(0, "");
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    setCurrentChatPeerName(item.value("name").toString());
    setCurrentChatPeerIcon(item.value("icon").toString());
    loadCurrentChatMessages();
    syncCurrentDialogDraft();
}

void AppController::selectContactIndex(int index)
{
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
        if (beforeTs <= 0) {
            beforeTs = _private_history_before_ts;
        }
        requestPrivateHistory(_current_chat_uid, beforeTs);
    }
}

void AppController::loadMoreContacts()
{
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

void AppController::sendTextMessage(const QString &text)
{
    if (_current_chat_uid <= 0 && _current_group_id <= 0) {
        return;
    }

    QString content = text;
    if (content.trimmed().isEmpty()) {
        return;
    }

    if (content.size() > 1024) {
        setTip("单条消息不能超过1024字符", true);
        return;
    }

    if (_current_group_id > 0) {
        if (!_reply_to_msg_id.isEmpty()) {
            content = MessageContentCodec::encodeReplyText(content,
                                                           _reply_to_msg_id,
                                                           _reply_target_name,
                                                           _reply_preview_text);
        }
        if (!dispatchGroupChatContent(content, QString())) {
            setTip("群消息发送失败", true);
        } else {
            cancelReplyMessage();
        }
        return;
    }

    if (!dispatchChatContent(content, content)) {
        setTip("消息发送失败", true);
    }
}

void AppController::sendImageMessage()
{
    if (_current_chat_uid <= 0 && _current_group_id <= 0) {
        return;
    }

    QString fileUrl;
    QString errorText;
    if (!LocalFilePickerService::pickImageUrl(&fileUrl, &errorText)) {
        if (!errorText.isEmpty()) {
            setTip(errorText, true);
        }
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setTip("用户状态异常，请重新登录", true);
        return;
    }

    UploadedMediaInfo uploaded;
    if (!MediaUploadService::uploadLocalFile(fileUrl, "image", selfInfo->_uid, &uploaded, &errorText)) {
        setTip(errorText.isEmpty() ? "图片上传失败" : errorText, true);
        return;
    }

    const QString encoded = MessageContentCodec::encodeImage(uploaded.remoteUrl);
    if (_current_group_id > 0) {
        if (!dispatchGroupChatContent(encoded, "[图片]")) {
            setTip("群图片发送失败", true);
        }
        return;
    }
    if (!dispatchChatContent(encoded, "[图片]")) {
        setTip("图片发送失败", true);
    }
}

void AppController::sendFileMessage()
{
    if (_current_chat_uid <= 0 && _current_group_id <= 0) {
        return;
    }

    QString fileUrl;
    QString fileName;
    QString errorText;
    if (!LocalFilePickerService::pickFileUrl(&fileUrl, &fileName, &errorText)) {
        if (!errorText.isEmpty()) {
            setTip(errorText, true);
        }
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setTip("用户状态异常，请重新登录", true);
        return;
    }

    UploadedMediaInfo uploaded;
    if (!MediaUploadService::uploadLocalFile(fileUrl, "file", selfInfo->_uid, &uploaded, &errorText)) {
        setTip(errorText.isEmpty() ? "文件上传失败" : errorText, true);
        return;
    }

    const QString remoteName = uploaded.fileName.isEmpty() ? fileName : uploaded.fileName;
    const QString encoded = MessageContentCodec::encodeFile(uploaded.remoteUrl, remoteName);
    const QString preview = remoteName.isEmpty() ? "[文件]" : QString("[文件] %1").arg(remoteName);
    if (_current_group_id > 0) {
        if (!dispatchGroupChatContent(encoded, preview)) {
            setTip("群文件发送失败", true);
        }
        return;
    }
    if (!dispatchChatContent(encoded, preview)) {
        setTip("文件发送失败", true);
    }
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
    QString errorText;
    if (!_contact_controller.sendSearchUser(uidText, &errorText)) {
        clearSearchResultOnly();
        setSearchStatus(errorText, true);
        return;
    }

    clearSearchResultOnly();
    setSearchPending(true);
    setSearchStatus("搜索中...", false);
}

void AppController::clearSearchState()
{
    clearSearchResultOnly();
    setSearchPending(false);
    setSearchStatus("", false);
}

void AppController::requestAddFriend(int uid, const QString &bakName, const QVariantList &labels)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }
    if (uid == selfInfo->_uid) {
        setSearchStatus("不能添加自己", true);
        return;
    }
    if (_gateway.userMgr()->CheckFriendById(uid)) {
        setSearchStatus("已是好友，无需重复申请", false);
        return;
    }

    _contact_controller.sendAddFriend(
        selfInfo->_uid, selfInfo->_name, uid, bakName, labels);
    setSearchStatus("好友申请已发送", false);
}

void AppController::approveFriend(int uid, const QString &backName, const QVariantList &labels)
{
    if (uid <= 0) {
        setAuthStatus("非法好友申请", true);
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setAuthStatus("用户状态异常，请重新登录", true);
        return;
    }

    if (_gateway.userMgr()->CheckFriendById(uid)) {
        _apply_request_model.markApproved(uid);
        setAuthStatus("已是好友", false);
        return;
    }

    QString remark = backName.trimmed();
    if (remark.isEmpty()) {
        remark = _apply_request_model.nameByUid(uid);
    }
    if (remark.isEmpty()) {
        remark = "MemoChat好友";
    }

    _contact_controller.sendApproveFriend(selfInfo->_uid, uid, remark, labels);
    _apply_request_model.setPending(uid, true);
    setAuthStatus("好友认证请求已发送", false);
}

void AppController::clearAuthStatus()
{
    setAuthStatus("", false);
}

void AppController::startVoiceChat()
{
    if (!ensureCallTargetFromCurrentChat()) {
        return;
    }
    sendCallInvite("voice");
}

void AppController::startVideoChat()
{
    if (!ensureCallTargetFromCurrentChat()) {
        return;
    }
    sendCallInvite("video");
}

void AppController::chooseAvatar()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setSettingsStatus("用户状态异常，请重新登录", true);
        return;
    }

    QString avatarUrl;
    QString errorText;
    if (!LocalFilePickerService::pickAvatarUrl(&avatarUrl, &errorText)) {
        if (!errorText.isEmpty()) {
            setSettingsStatus(errorText, true);
        }
        return;
    }

    _gateway.userMgr()->UpdateIcon(avatarUrl);
    const QString normalized = normalizeIconPath(avatarUrl);
    if (_current_user_icon != normalized) {
        _current_user_icon = normalized;
        emit currentUserChanged();
    }
    setSettingsStatus("已选择新头像，点击保存后同步", false);
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

    QString iconForSave = _current_user_icon;
    const QUrl iconUrl(iconForSave);
    if (iconUrl.isLocalFile() || QFileInfo(iconForSave).isAbsolute()) {
        UploadedMediaInfo uploaded;
        if (!MediaUploadService::uploadLocalFile(iconForSave, "avatar", selfInfo->_uid, &uploaded, &errorText)) {
            setSettingsStatus(errorText.isEmpty() ? "头像上传失败" : errorText, true);
            return;
        }
        if (uploaded.remoteUrl.isEmpty()) {
            setSettingsStatus("头像上传失败", true);
            return;
        }
        iconForSave = uploaded.remoteUrl;
    }

    _profile_controller.sendSaveProfile(
        selfInfo->_uid, selfInfo->_name, nick, desc, iconForSave);
    setSettingsStatus("资料同步中...", false);
}

void AppController::clearSettingsStatus()
{
    setSettingsStatus("", false);
}

void AppController::refreshGroupList()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setGroupStatus("用户状态异常，请重新登录", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_GET_GROUP_LIST_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_GET_DIALOG_LIST_REQ, payload);
}

void AppController::createGroup(const QString &name, const QVariantList &memberUserIdList)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        setGroupStatus("用户状态异常，请重新登录", true);
        return;
    }
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty() || trimmedName.size() > 64) {
        setGroupStatus("群名称长度需在1-64之间", true);
        return;
    }

    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    QJsonArray memberUserIds;
    for (const auto &one : memberUserIdList) {
        const QString userId = one.toString().trimmed();
        if (userId.isEmpty()) {
            continue;
        }
        if (!kUserIdPattern.match(userId).hasMatch()) {
            setGroupStatus(QString("成员ID格式非法：%1").arg(userId), true);
            return;
        }
        if (selfInfo->_user_id == userId) {
            continue;
        }
        memberUserIds.append(userId);
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["name"] = trimmedName;
    obj["member_limit"] = 200;
    obj["member_user_ids"] = memberUserIds;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_CREATE_GROUP_REQ, payload);
    setGroupStatus("正在创建群聊...", false);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_INVITE_GROUP_MEMBER_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_APPLY_JOIN_GROUP_REQ, payload);
    setGroupStatus("申请已发送", false);
}

void AppController::reviewGroupApply(qint64 applyId, bool agree)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || applyId <= 0) {
        setGroupStatus("审核参数非法", true);
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["apply_id"] = applyId;
    obj["agree"] = agree;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_REVIEW_GROUP_APPLY_REQ, payload);
    setGroupStatus("审核请求已发送", false);
}

void AppController::sendGroupTextMessage(const QString &text)
{
    if (_current_group_id <= 0) {
        setGroupStatus("请选择群聊", true);
        return;
    }
    sendTextMessage(text);
}

void AppController::sendGroupImageMessage()
{
    if (_current_group_id <= 0) {
        setGroupStatus("请选择群聊", true);
        return;
    }
    sendImageMessage();
}

void AppController::sendGroupFileMessage()
{
    if (_current_group_id <= 0) {
        setGroupStatus("请选择群聊", true);
        return;
    }
    sendFileMessage();
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
    _gateway.tcpMgr()->slot_send_data(_current_group_id > 0
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
    _gateway.tcpMgr()->slot_send_data(_current_group_id > 0
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
        _gateway.tcpMgr()->slot_send_data(ReqId::ID_FORWARD_PRIVATE_MSG_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_FORWARD_GROUP_MSG_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_GROUP_HISTORY_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_UPDATE_GROUP_ANNOUNCEMENT_REQ, payload);
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

    QString avatarUrl;
    QString errorText;
    if (!LocalFilePickerService::pickAvatarUrl(&avatarUrl, &errorText)) {
        if (!errorText.isEmpty()) {
            setGroupStatus(errorText, true);
        }
        return;
    }

    UploadedMediaInfo uploaded;
    if (!MediaUploadService::uploadLocalFile(avatarUrl, "group_avatar", selfInfo->_uid, &uploaded, &errorText)) {
        setGroupStatus(errorText.isEmpty() ? "群头像上传失败" : errorText, true);
        return;
    }
    if (uploaded.remoteUrl.isEmpty()) {
        setGroupStatus("群头像上传失败", true);
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["icon"] = uploaded.remoteUrl;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_UPDATE_GROUP_ICON_REQ, payload);
    setGroupStatus("群头像更新中...", false);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_SET_GROUP_ADMIN_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_MUTE_GROUP_MEMBER_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_KICK_GROUP_MEMBER_REQ, payload);
}

void AppController::quitCurrentGroup()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0) {
        setGroupStatus("请选择群聊", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_QUIT_GROUP_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_SYNC_DRAFT_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_PIN_DIALOG_REQ, payload);

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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_SYNC_DRAFT_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_SYNC_DRAFT_REQ, payload);
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
    if (!checkEmailValid(email)) {
        return;
    }

    if (!checkPwdValid(password)) {
        return;
    }

    setBusy(true);
    setTip("", false);
    _auth_controller.sendLogin(email, password);
}

void AppController::requestRegisterCode(const QString &email)
{
    if (!checkEmailValid(email)) {
        return;
    }

    setBusy(true);
    _auth_controller.sendVerifyCode(email, Modules::REGISTERMOD);
}

void AppController::registerUser(const QString &user, const QString &email, const QString &password,
                                 const QString &confirm, const QString &verifyCode)
{
    if (!checkUserValid(user)) {
        return;
    }
    if (!checkEmailValid(email)) {
        return;
    }
    if (!checkPwdValid(password)) {
        return;
    }
    if (password != confirm) {
        setTip("密码和确认密码不匹配", true);
        return;
    }
    if (!checkVerifyCodeValid(verifyCode)) {
        return;
    }

    setBusy(true);
    _auth_controller.sendRegister(user, email, password, confirm, verifyCode);
}

void AppController::requestResetCode(const QString &email)
{
    if (!checkEmailValid(email)) {
        return;
    }

    setBusy(true);
    _auth_controller.sendVerifyCode(email, Modules::RESETMOD);
}

void AppController::resetPassword(const QString &user, const QString &email, const QString &password,
                                  const QString &verifyCode)
{
    if (!checkUserValid(user)) {
        return;
    }
    if (!checkEmailValid(email)) {
        return;
    }
    if (!checkPwdValid(password)) {
        return;
    }
    if (!checkVerifyCodeValid(verifyCode)) {
        return;
    }

    setBusy(true);
    _auth_controller.sendResetPassword(user, email, password, verifyCode);
}

void AppController::onLoginHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (id != ReqId::ID_LOGIN_USER) {
        return;
    }

    if (err != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setBusy(false);
        setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setBusy(false);
        if (error == ErrorCodes::ERR_VERSION_TOO_LOW) {
            const QString minVersion = obj.value("min_version").toString(QStringLiteral("2.0.0"));
            setTip(QString("客户端版本过低，请升级到 %1 或以上").arg(minVersion), true);
            return;
        }

        switch (error) {
        case 1009:
            setTip("邮箱或密码错误", true);
            break;
        case 1001:
            setTip("登录参数格式错误", true);
            break;
        case 1002:
            setTip("登录服务暂不可用，请稍后重试", true);
            break;
        default:
            setTip(QString("登录失败（错误码:%1）").arg(error), true);
            break;
        }
        return;
    }

    ServerInfo server_info;
    server_info.Uid = obj.value("uid").toInt();
    server_info.Host = obj.value("host").toString();
    server_info.Port = obj.value("port").toString();
    server_info.Token = obj.value("token").toString();

    _pending_uid = server_info.Uid;
    _pending_token = server_info.Token;
    _pending_trace_id = obj.value("trace_id").toString();
    setTip("正在连接聊天服务...", false);
    _gateway.tcpMgr()->slot_tcp_connect(server_info);
}

void AppController::onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setBusy(false);
        setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("参数错误", true);
        return;
    }

    if (id == ReqId::ID_GET_VARIFY_CODE) {
        setBusy(false);
        setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_REG_USER) {
        setBusy(false);
        setTip("用户注册成功", false);
        if (!_register_success_page) {
            _register_success_page = true;
            emit registerSuccessPageChanged();
        }

        _register_countdown = 5;
        emit registerCountdownChanged();
        _register_countdown_timer.start(1000);
    }
}

void AppController::onResetHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setBusy(false);
        setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("参数错误", true);
        return;
    }

    setBusy(false);
    if (id == ReqId::ID_GET_VARIFY_CODE) {
        setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_RESET_PWD) {
        setTip("重置成功, 点击返回登录", false);
    }
}

void AppController::onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (id != ReqId::ID_UPDATE_PROFILE) {
        return;
    }

    if (err != ErrorCodes::SUCCESS) {
        setSettingsStatus("资料同步失败：网络错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setSettingsStatus("资料同步失败：响应解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setSettingsStatus("资料同步失败", true);
        return;
    }

    const QString nick = obj.value("nick").toString(_current_user_nick);
    const QString desc = obj.value("desc").toString(_current_user_desc);
    const QString icon = normalizeIconPath(obj.value("icon").toString(_current_user_icon));

    _gateway.userMgr()->UpdateNickAndDesc(nick, desc);
    _gateway.userMgr()->UpdateIcon(icon);

    bool changed = false;
    if (_current_user_nick != nick) {
        _current_user_nick = nick;
        changed = true;
    }
    if (_current_user_desc != desc) {
        _current_user_desc = desc;
        changed = true;
    }
    if (_current_user_icon != icon) {
        _current_user_icon = icon;
        changed = true;
    }

    if (changed) {
        emit currentUserChanged();
    }
    setSettingsStatus("资料已同步", false);
}

void AppController::onTcpConnectFinished(bool success)
{
    if (!success) {
        _chat_login_timeout_timer.stop();
        setBusy(false);
        setTip("网络异常", true);
        return;
    }

    setTip("聊天服务连接成功，正在登录...", false);
    QJsonObject obj;
    obj["uid"] = _pending_uid;
    obj["token"] = _pending_token;
    obj["protocol_version"] = 2;
    if (!_pending_trace_id.isEmpty()) {
        obj["trace_id"] = _pending_trace_id;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_CHAT_LOGIN, payload);
    _chat_login_timeout_timer.start();
}

void AppController::onChatLoginFailed(int err)
{
    _chat_login_timeout_timer.stop();
    setBusy(false);
    if (err == ErrorCodes::ERR_PROTOCOL_VERSION) {
        setTip("客户端版本过旧，请升级到 v2 协议客户端", true);
        return;
    }
    setTip(QString("登录失败, err is %1").arg(err), true);
}

void AppController::onSwitchToChat()
{
    _chat_login_timeout_timer.stop();
    setBusy(false);
    setTip("", false);
    setSearchPending(false);
    setChatLoadingMore(false);
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(false);
    _private_history_before_ts = 0;
    _private_history_pending_before_ts = 0;
    _private_history_pending_peer_uid = 0;
    _group_history_before_seq = 0;
    _group_history_has_more = true;
    _pending_group_msg_group_map.clear();
    _dialog_mention_map.clear();
    setPendingReplyContext(QString(), QString(), QString());
    setContactLoadingMore(false);
    setAuthStatus("", false);
    setSettingsStatus("", false);
    setContactPane(ApplyRequestPane);
    setCurrentContact(0, "", "", "qrc:/res/head_1.jpg", "", 0);
    setPage(ChatPage);

    auto user_info = _gateway.userMgr()->GetUserInfo();
    if (user_info) {
        _private_cache_store.openForUser(user_info->_uid);
        _group_cache_store.openForUser(user_info->_uid);
        loadDraftStore(user_info->_uid);
        const QString name = user_info->_name;
        const QString nick = user_info->_nick;
        const QString icon = normalizeIconPath(user_info->_icon);
        const QString desc = user_info->_desc;
        const QString userId = user_info->_user_id;

        if (_current_user_name != name) {
            _current_user_name = name;
        }
        if (_current_user_nick != nick) {
            _current_user_nick = nick;
        }
        if (_current_user_icon != icon) {
            _current_user_icon = icon;
        }
        if (_current_user_desc != desc) {
            _current_user_desc = desc;
        }
        if (_current_user_id != userId) {
            _current_user_id = userId;
        }
        emit currentUserChanged();
    } else {
        _dialog_draft_map.clear();
        _dialog_server_mute_map.clear();
        _dialog_mention_map.clear();
        setCurrentDraftText("");
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
    }

    QTimer::singleShot(0, this, [this]() {
        if (_page != ChatPage) {
            return;
        }

        refreshFriendModels();
        refreshApplyModel();
        refreshGroupList();
        requestDialogList();
        if (_chat_list_model.count() > 0) {
            selectChatIndex(0);
        } else {
            _current_chat_uid = 0;
            setCurrentChatPeerName("");
            setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
            setCurrentDraftText("");
            setCurrentDialogPinned(false);
            setCurrentDialogMuted(false);
        }

        // Keep chat session alive; server considers session expired after heartbeat timeout.
        _heartbeat_timer.start(10000);
        onHeartbeatTimeout();
    });
}

void AppController::onRegisterCountdownTimeout()
{
    if (_register_countdown > 0) {
        --_register_countdown;
        emit registerCountdownChanged();
    }

    if (_register_countdown <= 0) {
        _register_countdown_timer.stop();
        _register_success_page = false;
        emit registerSuccessPageChanged();
        switchToLogin();
    }
}

void AppController::onHeartbeatTimeout()
{
    if (_page != ChatPage) {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }

    QJsonObject hb;
    hb["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(hb).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_HEART_BEAT_REQ, payload);
}

void AppController::onAddAuthFriend(std::shared_ptr<AuthInfo> authInfo)
{
    if (authInfo) {
        _gateway.userMgr()->AddFriend(authInfo);
    }
    _chat_list_model.upsertFriend(authInfo);
    _contact_list_model.upsertFriend(authInfo);
    refreshDialogModel();
    refreshChatLoadMoreState();
    refreshContactLoadMoreState();
    if (authInfo) {
        _gateway.userMgr()->MarkApplyStatus(authInfo->_uid, 1);
        _apply_request_model.markApproved(authInfo->_uid);
        if (_current_contact_uid == authInfo->_uid) {
            setCurrentContact(authInfo->_uid, authInfo->_name, authInfo->_nick, authInfo->_icon,
                              authInfo->_nick, authInfo->_sex, authInfo->_user_id);
        }
    }
}

void AppController::onAuthRsp(std::shared_ptr<AuthRsp> authRsp)
{
    if (authRsp) {
        _gateway.userMgr()->AddFriend(authRsp);
    }
    _chat_list_model.upsertFriend(authRsp);
    _contact_list_model.upsertFriend(authRsp);
    refreshDialogModel();
    refreshChatLoadMoreState();
    refreshContactLoadMoreState();
    if (authRsp) {
        _gateway.userMgr()->MarkApplyStatus(authRsp->_uid, 1);
        _apply_request_model.markApproved(authRsp->_uid);
        setAuthStatus("好友添加成功", false);
        if (_current_contact_uid == authRsp->_uid) {
            setCurrentContact(authRsp->_uid, authRsp->_name, authRsp->_nick, authRsp->_icon,
                              authRsp->_nick, authRsp->_sex, authRsp->_user_id);
        }
    }
}

void AppController::onTextChatMsg(std::shared_ptr<TextChatMsg> msg)
{
    if (!msg || msg->_chat_msgs.empty()) {
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }

    const int selfUid = selfInfo->_uid;
    const bool fromSelf = (msg->_from_uid == selfUid);
    const int peerUid = (msg->_from_uid == selfUid) ? msg->_to_uid : msg->_from_uid;
    qint64 latestPeerTs = 0;
    for (const auto &one : msg->_chat_msgs) {
        if (one && (one->_deleted_at_ms > 0 || one->_msg_content == QStringLiteral("[消息已撤回]"))) {
            one->_msg_state = QStringLiteral("deleted");
        } else if (one && one->_edited_at_ms > 0) {
            one->_msg_state = QStringLiteral("edited");
        }
        if (one && one->_from_uid == peerUid) {
            latestPeerTs = qMax(latestPeerTs, one->_created_at);
        }
    }
    _gateway.userMgr()->AppendFriendChatMsg(peerUid, msg->_chat_msgs);
    _private_cache_store.upsertMessages(selfUid, peerUid, msg->_chat_msgs);
    const auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    QString preview;
    if (friendInfo && !friendInfo->_chat_msgs.empty()) {
        preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
    } else {
        preview = MessageContentCodec::toPreviewText(msg->_chat_msgs.back()->_msg_content);
    }
    qint64 lastTs = QDateTime::currentMSecsSinceEpoch();
    if (!msg->_chat_msgs.empty() && msg->_chat_msgs.back()) {
        lastTs = msg->_chat_msgs.back()->_created_at;
    }
    _chat_list_model.updateLastMessage(peerUid, preview, lastTs);
    _dialog_list_model.updateLastMessage(peerUid, preview, lastTs);
    const bool isViewingCurrentPrivate = (_current_group_id <= 0 && peerUid == _current_chat_uid);
    if (isViewingCurrentPrivate) {
        _dialog_list_model.clearUnread(peerUid);
        if (latestPeerTs > 0) {
            sendPrivateReadAck(peerUid, latestPeerTs);
        }
    } else if (!fromSelf) {
        _dialog_list_model.incrementUnread(peerUid);
    }

    if (peerUid != _current_chat_uid) {
        return;
    }

    for (const auto &chat : msg->_chat_msgs) {
        _message_model.appendMessage(chat, selfUid);
    }
    _private_history_before_ts = _message_model.earliestCreatedAt();
}

void AppController::onUserSearch(std::shared_ptr<SearchInfo> searchInfo)
{
    setSearchPending(false);

    if (!searchInfo) {
        clearSearchResultOnly();
        setSearchStatus("未找到该用户", true);
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        clearSearchResultOnly();
        setSearchStatus("用户状态异常，请重新登录", true);
        return;
    }

    if (searchInfo->_uid == selfInfo->_uid) {
        clearSearchResultOnly();
        setSearchStatus("不能搜索自己", true);
        return;
    }

    if (_gateway.userMgr()->CheckFriendById(searchInfo->_uid)) {
        clearSearchResultOnly();
        setSearchStatus("已是好友，已切换到会话", false);
        selectChatByUid(searchInfo->_uid);
        switchChatTab(ChatTabPage);
        return;
    }

    _search_result_model.setResult(searchInfo);
    setSearchStatus("已找到用户", false);
}

void AppController::onFriendApply(std::shared_ptr<AddFriendApply> applyInfo)
{
    if (!applyInfo) {
        return;
    }

    if (_gateway.userMgr()->AlreadyApply(applyInfo->_from_uid)) {
        return;
    }

    auto apply = std::make_shared<ApplyInfo>(applyInfo);
    _gateway.userMgr()->AddApplyList(apply);
    _apply_request_model.upsertApply(apply);
    setAuthStatus(QString("收到来自 %1 的好友申请").arg(applyInfo->_name), false);
}

void AppController::onNotifyOffline()
{
    _chat_login_timeout_timer.stop();
    if (_page != ChatPage) {
        return;
    }

    _heartbeat_timer.stop();
    _gateway.tcpMgr()->CloseConnection();
    switchToLogin();
    setTip("同账号异地登录，该终端下线", true);
}

void AppController::onConnectionClosed()
{
    _chat_login_timeout_timer.stop();
    if (_page != ChatPage) {
        if (_busy) {
            setBusy(false);
            setTip("聊天服务连接断开，请重试", true);
        }
        return;
    }

    _heartbeat_timer.stop();
    switchToLogin();
    setTip("心跳超时或连接断开，请重新登录", true);
}

void AppController::onGroupListUpdated()
{
    refreshGroupModel();
    refreshDialogModel();
    requestDialogList();
    if (_current_group_id > 0) {
        auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
        if (groupInfo) {
            setCurrentGroup(groupInfo->_group_id, groupInfo->_name, groupInfo->_group_code);
            const QString currentGroupIcon = groupInfo->_icon.trimmed().isEmpty()
                ? QStringLiteral("qrc:/res/chat_icon.png")
                : groupInfo->_icon;
            setCurrentChatPeerIcon(currentGroupIcon);
            _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
            syncCurrentDialogDraft();
            return;
        }

        setCurrentGroup(0, "");
        if (_current_chat_uid > 0) {
            loadCurrentChatMessages();
        } else {
            _message_model.clear();
            setCurrentChatPeerName("");
            setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
            setCurrentDraftText("");
            setCurrentDialogPinned(false);
            setCurrentDialogMuted(false);
        }
    }
}

void AppController::onGroupInvite(qint64 groupId, QString groupCode, QString groupName, int operatorUid)
{
    Q_UNUSED(operatorUid);
    setGroupStatus(QString("收到群邀请：%1").arg(groupName), false);
    QJsonObject req;
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }
    req["fromuid"] = selfInfo->_uid;
    const QByteArray payload = QJsonDocument(req).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_GET_GROUP_LIST_REQ, payload);
    if (_current_group_id <= 0) {
        setCurrentGroup(groupId, groupName, groupCode);
    }
}

void AppController::onGroupApply(qint64 groupId, int applicantUid, QString applicantUserId, QString reason)
{
    Q_UNUSED(groupId);
    const QString displayId = applicantUserId.isEmpty() ? QString::number(applicantUid) : applicantUserId;
    setGroupStatus(QString("收到入群申请：%1 %2").arg(displayId).arg(reason), false);
}

void AppController::onGroupMemberChanged(QJsonObject payload)
{
    const QString event = payload.value("event").toString();
    if (!event.isEmpty() && event != "group_read_ack") {
        setGroupStatus(QString("群事件：%1").arg(event), false);
    }

    if (event == "group_read_ack") {
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        if (!selfInfo) {
            return;
        }
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const int readerUid = payload.value("fromuid").toInt();
        if (groupId <= 0 || readerUid <= 0 || readerUid == selfInfo->_uid) {
            return;
        }
        qint64 readTs = payload.value("read_ts").toVariant().toLongLong();
        if (readTs <= 0) {
            readTs = QDateTime::currentMSecsSinceEpoch();
        } else if (readTs < 100000000000LL) {
            readTs *= 1000;
        }
        if (_gateway.userMgr()->MarkGroupOutgoingReadUntil(groupId, selfInfo->_uid, readTs) <= 0) {
            return;
        }
        auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
        if (groupInfo && _group_cache_store.isReady()) {
            _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
        }
        if (groupId == _current_group_id && groupInfo) {
            _message_model.setMessages(groupInfo->_chat_msgs, selfInfo->_uid);
        }
        return;
    }

    if (event == "group_msg_edited" || event == "group_msg_revoked") {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const QString msgId = payload.value("msgid").toString();
        const QString content = payload.value("content").toString();
        qint64 editedAtMs = payload.value("edited_at_ms").toVariant().toLongLong();
        qint64 deletedAtMs = payload.value("deleted_at_ms").toVariant().toLongLong();
        if (editedAtMs > 0 && editedAtMs < 100000000000LL) {
            editedAtMs *= 1000;
        }
        if (deletedAtMs > 0 && deletedAtMs < 100000000000LL) {
            deletedAtMs *= 1000;
        }
        if (groupId > 0 && !msgId.isEmpty() && !content.isEmpty()) {
            const QString state = (event == "group_msg_revoked") ? QStringLiteral("deleted") : QStringLiteral("edited");
            _gateway.userMgr()->UpdateGroupChatMsgContent(groupId, msgId, content, state, editedAtMs, deletedAtMs);
            auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
            if (groupInfo) {
                auto selfInfo = _gateway.userMgr()->GetUserInfo();
                if (selfInfo && _group_cache_store.isReady()) {
                    _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
                }
                if (groupId == _current_group_id) {
                    _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
                }
            }
        }
        return;
    }

    if (event == "group_icon_updated"
        && payload.value("groupid").toVariant().toLongLong() == _current_group_id) {
        const QString newGroupIcon = payload.value("icon").toString().trimmed().isEmpty()
            ? QStringLiteral("qrc:/res/chat_icon.png")
            : payload.value("icon").toString();
        setCurrentChatPeerIcon(newGroupIcon);
    }
    refreshGroupList();
}

void AppController::onGroupChatMsg(std::shared_ptr<GroupChatMsg> msg)
{
    if (!msg || !msg->_msg) {
        return;
    }

    QString messageState = QStringLiteral("sent");
    if (msg->_msg->_deleted_at_ms > 0 || msg->_msg->_msg_type == QStringLiteral("revoke")
        || msg->_msg->_msg_content == QStringLiteral("[消息已撤回]")) {
        messageState = QStringLiteral("deleted");
    } else if (msg->_msg->_edited_at_ms > 0) {
        messageState = QStringLiteral("edited");
    }
    auto textMsg = std::make_shared<TextChatData>(msg->_msg->_msg_id, msg->_msg->_msg_content,
                                                  msg->_msg->_from_uid, static_cast<int>(msg->_group_id),
                                                  msg->_from_name, msg->_msg->_created_at, msg->_from_icon,
                                                  messageState,
                                                  msg->_msg->_server_msg_id,
                                                  msg->_msg->_group_seq,
                                                  msg->_msg->_reply_to_server_msg_id,
                                                  msg->_msg->_forward_meta_json,
                                                  msg->_msg->_edited_at_ms,
                                                  msg->_msg->_deleted_at_ms);
    _gateway.userMgr()->UpsertGroupChatMsg(msg->_group_id, textMsg);
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (selfInfo && _group_cache_store.isReady()) {
        _group_cache_store.upsertMessages(selfInfo->_uid, msg->_group_id, {textMsg});
    }

    int pseudoUid = 0;
    for (auto it = _group_uid_map.cbegin(); it != _group_uid_map.cend(); ++it) {
        if (it.value() == msg->_group_id) {
            pseudoUid = it.key();
            break;
        }
    }
    if (pseudoUid == 0) {
        pseudoUid = -static_cast<int>(msg->_group_id);
        _group_uid_map.insert(pseudoUid, msg->_group_id);
    }
    if (pseudoUid != 0) {
        const QString preview = MessageContentCodec::toPreviewText(msg->_msg->_msg_content);
        const int selfUid = _gateway.userMgr()->GetUid();
        const bool fromSelf = (msg->_from_uid == selfUid);
        bool mentionedMe = false;
        if (!fromSelf && msg->_msg->_mention_all) {
            mentionedMe = true;
        } else if (!fromSelf && !msg->_msg->_mentions.isEmpty()) {
            for (const auto &one : msg->_msg->_mentions) {
                if (one.toInt() == selfUid) {
                    mentionedMe = true;
                    break;
                }
            }
        }
        _group_list_model.updateLastMessage(pseudoUid, preview, msg->_msg->_created_at);
        _dialog_list_model.updateLastMessage(
            pseudoUid,
            mentionedMe ? QString("[有人@你] %1").arg(preview) : preview,
            msg->_msg->_created_at);
        const bool isViewingCurrentGroup = (_current_group_id == msg->_group_id && _current_chat_uid <= 0);
        if (isViewingCurrentGroup) {
            _dialog_list_model.clearUnread(pseudoUid);
            _dialog_list_model.clearMention(pseudoUid);
            _dialog_mention_map.remove(pseudoUid);
        } else if (!fromSelf) {
            _dialog_list_model.incrementUnread(pseudoUid);
            if (mentionedMe) {
                const int nextMentionCount = _dialog_mention_map.value(pseudoUid, 0) + 1;
                _dialog_mention_map.insert(pseudoUid, nextMentionCount);
                _dialog_list_model.setMentionCount(pseudoUid, nextMentionCount);
            }
        }
    }

    if (msg->_group_id != _current_group_id) {
        return;
    }
    _message_model.appendMessage(textMsg, _gateway.userMgr()->GetUid());
    sendGroupReadAck(msg->_group_id, textMsg->_created_at);
}

void AppController::onGroupRsp(ReqId reqId, int error, QJsonObject payload)
{
    auto action_text = [](ReqId id) -> QString {
        switch (id) {
        case ID_CREATE_GROUP_RSP: return "建群";
        case ID_GET_GROUP_LIST_RSP: return "刷新群列表";
        case ID_INVITE_GROUP_MEMBER_RSP: return "邀请成员";
        case ID_APPLY_JOIN_GROUP_RSP: return "申请入群";
        case ID_REVIEW_GROUP_APPLY_RSP: return "审核申请";
        case ID_GROUP_CHAT_MSG_RSP: return "发送群消息";
        case ID_GROUP_HISTORY_RSP: return "拉取群历史";
        case ID_EDIT_GROUP_MSG_RSP: return "编辑群消息";
        case ID_REVOKE_GROUP_MSG_RSP: return "撤回群消息";
        case ID_FORWARD_GROUP_MSG_RSP: return "转发群消息";
        case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP: return "更新群公告";
        case ID_UPDATE_GROUP_ICON_RSP: return "更新群头像";
        case ID_MUTE_GROUP_MEMBER_RSP: return "禁言成员";
        case ID_SET_GROUP_ADMIN_RSP: return "设置管理员";
        case ID_KICK_GROUP_MEMBER_RSP: return "踢出成员";
        case ID_QUIT_GROUP_RSP: return "退出群聊";
        case ID_SYNC_DRAFT_RSP: return "同步草稿";
        case ID_PIN_DIALOG_RSP: return "置顶会话";
        case ID_EDIT_PRIVATE_MSG_RSP: return "编辑私聊消息";
        case ID_REVOKE_PRIVATE_MSG_RSP: return "撤回私聊消息";
        case ID_FORWARD_PRIVATE_MSG_RSP: return "转发私聊消息";
        default: return "群操作";
        }
    };

    if (error != ErrorCodes::SUCCESS) {
        if (reqId == ID_GROUP_HISTORY_RSP) {
            setPrivateHistoryLoading(false);
        }
        if (reqId == ID_GROUP_CHAT_MSG_RSP) {
            QString clientMsgId = payload.value("client_msg_id").toString();
            if (clientMsgId.isEmpty()) {
                clientMsgId = payload.value("msg").toObject().value("msgid").toString();
            }
            qint64 groupId = payload.value("groupid").toVariant().toLongLong();
            if (groupId <= 0 && !clientMsgId.isEmpty()) {
                groupId = _pending_group_msg_group_map.value(clientMsgId, 0);
            }

            if (groupId > 0 && !clientMsgId.isEmpty()) {
                if (_gateway.userMgr()->UpdateGroupChatMsgState(groupId, clientMsgId, "failed")
                    && groupId == _current_group_id) {
                    _message_model.updateMessageState(clientMsgId, "failed");
                }
                _pending_group_msg_group_map.remove(clientMsgId);
            }
        }
        if (reqId == ID_SYNC_DRAFT_RSP) {
            return;
        }
        if (reqId == ID_EDIT_PRIVATE_MSG_RSP
            || reqId == ID_REVOKE_PRIVATE_MSG_RSP
            || reqId == ID_FORWARD_PRIVATE_MSG_RSP) {
            setTip(QString("%1失败（错误码:%2）").arg(action_text(reqId)).arg(error), true);
        } else {
            setGroupStatus(QString("%1失败（错误码:%2）").arg(action_text(reqId)).arg(error), true);
        }
        return;
    }

    switch (reqId) {
    case ID_CREATE_GROUP_RSP: {
        const QString groupName = payload.value("name").toString();
        setGroupStatus(groupName.isEmpty() ? "群聊创建成功" : QString("群聊创建成功：%1").arg(groupName), false);
        refreshGroupList();
        break;
    }
    case ID_INVITE_GROUP_MEMBER_RSP:
        setGroupStatus("邀请成员成功", false);
        refreshGroupList();
        break;
    case ID_APPLY_JOIN_GROUP_RSP:
        setGroupStatus("入群申请已提交", false);
        break;
    case ID_REVIEW_GROUP_APPLY_RSP:
        setGroupStatus("审核操作已完成", false);
        refreshGroupList();
        break;
    case ID_GROUP_HISTORY_RSP: {
        setPrivateHistoryLoading(false);
        _group_history_has_more = payload.value("has_more").toBool(false);
        setCanLoadMorePrivateHistory(_group_history_has_more);
        _group_history_before_seq = payload.value("next_before_seq").toVariant().toLongLong();
        if (_current_group_id > 0) {
            auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
            if (groupInfo) {
                _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
                if (_group_history_before_seq <= 0) {
                    for (const auto &one : groupInfo->_chat_msgs) {
                        if (!one || one->_group_seq <= 0) {
                            continue;
                        }
                        if (_group_history_before_seq <= 0 || one->_group_seq < _group_history_before_seq) {
                            _group_history_before_seq = one->_group_seq;
                        }
                    }
                }
                qint64 readTs = 0;
                if (!groupInfo->_chat_msgs.empty() && groupInfo->_chat_msgs.back()) {
                    readTs = groupInfo->_chat_msgs.back()->_created_at;
                }
                sendGroupReadAck(_current_group_id, readTs);
                const int dialogUid = -static_cast<int>(_current_group_id);
                _dialog_list_model.clearMention(dialogUid);
                _dialog_mention_map.remove(dialogUid);
                auto selfInfo = _gateway.userMgr()->GetUserInfo();
                if (selfInfo && _group_cache_store.isReady()) {
                    _group_cache_store.upsertMessages(selfInfo->_uid, _current_group_id, groupInfo->_chat_msgs);
                }
            }
        }
        setGroupStatus("历史消息已加载", false);
        break;
    }
    case ID_EDIT_GROUP_MSG_RSP:
    case ID_REVOKE_GROUP_MSG_RSP: {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const QString msgId = payload.value("msgid").toString();
        const QString content = payload.value("content").toString();
        qint64 editedAtMs = payload.value("edited_at_ms").toVariant().toLongLong();
        qint64 deletedAtMs = payload.value("deleted_at_ms").toVariant().toLongLong();
        if (editedAtMs > 0 && editedAtMs < 100000000000LL) {
            editedAtMs *= 1000;
        }
        if (deletedAtMs > 0 && deletedAtMs < 100000000000LL) {
            deletedAtMs *= 1000;
        }
        if (groupId > 0 && !msgId.isEmpty() && !content.isEmpty()) {
            const QString state = (reqId == ID_REVOKE_GROUP_MSG_RSP) ? QStringLiteral("deleted") : QStringLiteral("edited");
            _gateway.userMgr()->UpdateGroupChatMsgContent(groupId, msgId, content, state, editedAtMs, deletedAtMs);
            auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
            if (groupInfo) {
                auto selfInfo = _gateway.userMgr()->GetUserInfo();
                if (selfInfo && _group_cache_store.isReady()) {
                    _group_cache_store.upsertMessages(selfInfo->_uid, groupId, groupInfo->_chat_msgs);
                }
                if (groupId == _current_group_id) {
                    _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
                }
            }
        }
        setGroupStatus(reqId == ID_EDIT_GROUP_MSG_RSP ? "群消息已编辑" : "群消息已撤回", false);
        break;
    }
    case ID_EDIT_PRIVATE_MSG_RSP:
    case ID_REVOKE_PRIVATE_MSG_RSP: {
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        if (!selfInfo) {
            break;
        }
        const int peerUid = payload.value("peer_uid").toInt();
        const QString msgId = payload.value("msgid").toString();
        const QString content = payload.value("content").toString();
        qint64 editedAtMs = payload.value("edited_at_ms").toVariant().toLongLong();
        qint64 deletedAtMs = payload.value("deleted_at_ms").toVariant().toLongLong();
        if (editedAtMs > 0 && editedAtMs < 100000000000LL) {
            editedAtMs *= 1000;
        }
        if (deletedAtMs > 0 && deletedAtMs < 100000000000LL) {
            deletedAtMs *= 1000;
        }
        const QString state = (reqId == ID_REVOKE_PRIVATE_MSG_RSP) ? QStringLiteral("deleted") : QStringLiteral("edited");
        if (peerUid > 0 && !msgId.isEmpty() && !content.isEmpty()) {
            _gateway.userMgr()->UpdatePrivateChatMsgContent(peerUid, msgId, content, state, editedAtMs, deletedAtMs);
            auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
            if (friendInfo && _private_cache_store.isReady()) {
                _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, friendInfo->_chat_msgs);
                if (!friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back()) {
                    const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
                    const qint64 lastTs = friendInfo->_chat_msgs.back()->_created_at;
                    _chat_list_model.updateLastMessage(peerUid, preview, lastTs);
                    _dialog_list_model.updateLastMessage(peerUid, preview, lastTs);
                }
            }
            if (peerUid == _current_chat_uid && friendInfo) {
                _message_model.setMessages(friendInfo->_chat_msgs, selfInfo->_uid);
            }
        }
        setTip(reqId == ID_EDIT_PRIVATE_MSG_RSP ? "私聊消息已编辑" : "私聊消息已撤回", false);
        break;
    }
    case ID_FORWARD_PRIVATE_MSG_RSP: {
        auto selfInfo = _gateway.userMgr()->GetUserInfo();
        if (!selfInfo) {
            break;
        }
        const int peerUid = payload.value("peer_uid").toInt();
        const QJsonObject msgObj = payload.value("msg").toObject();
        QString msgId = payload.value("client_msg_id").toString();
        if (msgId.isEmpty()) {
            msgId = msgObj.value("msgid").toString();
        }
        qint64 createdAt = payload.value("created_at").toVariant().toLongLong();
        if (createdAt <= 0) {
            createdAt = msgObj.value("created_at").toVariant().toLongLong();
        }
        if (createdAt <= 0) {
            createdAt = QDateTime::currentMSecsSinceEpoch();
        } else if (createdAt < 100000000000LL) {
            createdAt *= 1000;
        }
        qint64 replyToServerMsgId = payload.value("reply_to_server_msg_id").toVariant().toLongLong();
        if (replyToServerMsgId <= 0) {
            replyToServerMsgId = msgObj.value("reply_to_server_msg_id").toVariant().toLongLong();
        }
        QString forwardMetaJson;
        const QJsonValue payloadForwardMeta = payload.value("forward_meta");
        if (payloadForwardMeta.isObject()) {
            forwardMetaJson = QString::fromUtf8(QJsonDocument(payloadForwardMeta.toObject()).toJson(QJsonDocument::Compact));
        } else if (payloadForwardMeta.isArray()) {
            forwardMetaJson = QString::fromUtf8(QJsonDocument(payloadForwardMeta.toArray()).toJson(QJsonDocument::Compact));
        } else if (payloadForwardMeta.isString()) {
            forwardMetaJson = payloadForwardMeta.toString();
        }
        if (forwardMetaJson.isEmpty()) {
            const QJsonValue msgForwardMeta = msgObj.value("forward_meta");
            if (msgForwardMeta.isObject()) {
                forwardMetaJson = QString::fromUtf8(QJsonDocument(msgForwardMeta.toObject()).toJson(QJsonDocument::Compact));
            } else if (msgForwardMeta.isArray()) {
                forwardMetaJson = QString::fromUtf8(QJsonDocument(msgForwardMeta.toArray()).toJson(QJsonDocument::Compact));
            } else if (msgForwardMeta.isString()) {
                forwardMetaJson = msgForwardMeta.toString();
            }
        }
        qint64 editedAtMs = payload.value("edited_at_ms").toVariant().toLongLong();
        if (editedAtMs <= 0) {
            editedAtMs = msgObj.value("edited_at_ms").toVariant().toLongLong();
        }
        if (editedAtMs > 0 && editedAtMs < 100000000000LL) {
            editedAtMs *= 1000;
        }
        qint64 deletedAtMs = payload.value("deleted_at_ms").toVariant().toLongLong();
        if (deletedAtMs <= 0) {
            deletedAtMs = msgObj.value("deleted_at_ms").toVariant().toLongLong();
        }
        if (deletedAtMs > 0 && deletedAtMs < 100000000000LL) {
            deletedAtMs *= 1000;
        }

        if (peerUid > 0 && !msgId.isEmpty()) {
            QString state = QStringLiteral("sent");
            if (deletedAtMs > 0 || msgObj.value("content").toString() == QStringLiteral("[消息已撤回]")) {
                state = QStringLiteral("deleted");
            } else if (editedAtMs > 0) {
                state = QStringLiteral("edited");
            }
            auto forwardedMsg = std::make_shared<TextChatData>(msgId,
                                                               msgObj.value("content").toString(),
                                                               selfInfo->_uid,
                                                               peerUid,
                                                               QString(),
                                                               createdAt,
                                                               QString(),
                                                               state,
                                                               0,
                                                               0,
                                                               replyToServerMsgId,
                                                               forwardMetaJson,
                                                               editedAtMs,
                                                               deletedAtMs);
            _gateway.userMgr()->AppendFriendChatMsg(peerUid, {forwardedMsg});
            if (_private_cache_store.isReady()) {
                _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, {forwardedMsg});
            }
            auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
            if (friendInfo && !friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back()) {
                const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
                const qint64 lastTs = friendInfo->_chat_msgs.back()->_created_at;
                _chat_list_model.updateLastMessage(peerUid, preview, lastTs);
                _dialog_list_model.updateLastMessage(peerUid, preview, lastTs);
            }
            if (_current_group_id <= 0 && peerUid == _current_chat_uid) {
                _message_model.upsertMessage(forwardedMsg, selfInfo->_uid);
            }
        }
        setTip("消息已转发", false);
        break;
    }
    case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP:
        setGroupStatus("群公告已更新", false);
        refreshGroupList();
        break;
    case ID_UPDATE_GROUP_ICON_RSP:
        if (_current_group_id == payload.value("groupid").toVariant().toLongLong()) {
            const QString updatedGroupIcon = payload.value("icon").toString().trimmed().isEmpty()
                ? QStringLiteral("qrc:/res/chat_icon.png")
                : payload.value("icon").toString();
            setCurrentChatPeerIcon(updatedGroupIcon);
        }
        setGroupStatus("群头像已更新", false);
        refreshGroupList();
        break;
    case ID_MUTE_GROUP_MEMBER_RSP:
        setGroupStatus("禁言设置已生效", false);
        break;
    case ID_SET_GROUP_ADMIN_RSP:
        setGroupStatus("管理员设置已更新", false);
        refreshGroupList();
        break;
    case ID_KICK_GROUP_MEMBER_RSP:
        setGroupStatus("成员已移出群聊", false);
        refreshGroupList();
        break;
    case ID_QUIT_GROUP_RSP:
        setGroupStatus("已退出当前群聊", false);
        if (_current_group_id > 0) {
            const int dialogUid = -static_cast<int>(_current_group_id);
            _dialog_mention_map.remove(dialogUid);
            _dialog_list_model.clearMention(dialogUid);
        }
        refreshGroupList();
        setCurrentGroup(0, "");
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        setPendingReplyContext(QString(), QString(), QString());
        if (_current_chat_uid > 0) {
            loadCurrentChatMessages();
        } else {
            _message_model.clear();
            setCurrentChatPeerName("");
            setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
            setCurrentDialogMuted(false);
        }
        break;
    case ID_GROUP_CHAT_MSG_RSP:
    case ID_FORWARD_GROUP_MSG_RSP: {
        const qint64 groupId = payload.value("groupid").toVariant().toLongLong();
        const QJsonObject msgObj = payload.value("msg").toObject();
        QString msgId = payload.value("client_msg_id").toString();
        if (msgId.isEmpty()) {
            msgId = msgObj.value("msgid").toString();
        }
        if (groupId > 0 && !msgId.isEmpty()) {
            qint64 createdAt = payload.value("created_at").toVariant().toLongLong();
            if (createdAt <= 0) {
                createdAt = msgObj.value("created_at").toVariant().toLongLong();
            }
            if (createdAt <= 0) {
                createdAt = QDateTime::currentMSecsSinceEpoch();
            } else if (createdAt < 100000000000LL) {
                createdAt *= 1000;
            }
            qint64 serverMsgId = payload.value("server_msg_id").toVariant().toLongLong();
            if (serverMsgId <= 0) {
                serverMsgId = msgObj.value("server_msg_id").toVariant().toLongLong();
            }
            qint64 groupSeq = payload.value("group_seq").toVariant().toLongLong();
            if (groupSeq <= 0) {
                groupSeq = msgObj.value("group_seq").toVariant().toLongLong();
            }
            qint64 replyToServerMsgId = payload.value("reply_to_server_msg_id").toVariant().toLongLong();
            if (replyToServerMsgId <= 0) {
                replyToServerMsgId = msgObj.value("reply_to_server_msg_id").toVariant().toLongLong();
            }
            QString forwardMetaJson;
            const QJsonValue payloadForwardMeta = payload.value("forward_meta");
            if (payloadForwardMeta.isObject()) {
                forwardMetaJson = QString::fromUtf8(QJsonDocument(payloadForwardMeta.toObject()).toJson(QJsonDocument::Compact));
            } else if (payloadForwardMeta.isArray()) {
                forwardMetaJson = QString::fromUtf8(QJsonDocument(payloadForwardMeta.toArray()).toJson(QJsonDocument::Compact));
            } else if (payloadForwardMeta.isString()) {
                forwardMetaJson = payloadForwardMeta.toString();
            }
            if (forwardMetaJson.isEmpty()) {
                const QJsonValue msgForwardMeta = msgObj.value("forward_meta");
                if (msgForwardMeta.isObject()) {
                    forwardMetaJson = QString::fromUtf8(QJsonDocument(msgForwardMeta.toObject()).toJson(QJsonDocument::Compact));
                } else if (msgForwardMeta.isArray()) {
                    forwardMetaJson = QString::fromUtf8(QJsonDocument(msgForwardMeta.toArray()).toJson(QJsonDocument::Compact));
                } else if (msgForwardMeta.isString()) {
                    forwardMetaJson = msgForwardMeta.toString();
                }
            }
            qint64 editedAtMs = payload.value("edited_at_ms").toVariant().toLongLong();
            if (editedAtMs <= 0) {
                editedAtMs = msgObj.value("edited_at_ms").toVariant().toLongLong();
            }
            if (editedAtMs > 0 && editedAtMs < 100000000000LL) {
                editedAtMs *= 1000;
            }
            qint64 deletedAtMs = payload.value("deleted_at_ms").toVariant().toLongLong();
            if (deletedAtMs <= 0) {
                deletedAtMs = msgObj.value("deleted_at_ms").toVariant().toLongLong();
            }
            if (deletedAtMs > 0 && deletedAtMs < 100000000000LL) {
                deletedAtMs *= 1000;
            }

            auto selfInfo = _gateway.userMgr()->GetUserInfo();
            const int selfUid = selfInfo ? selfInfo->_uid : _gateway.userMgr()->GetUid();
            const QString senderName = payload.value("from_nick").toString(
                payload.value("from_name").toString(selfInfo ? selfInfo->_nick : QString()));
            const QString senderIcon = normalizeIconPath(payload.value("from_icon").toString(
                selfInfo ? selfInfo->_icon : QString()));
            QString state = QStringLiteral("sent");
            if (deletedAtMs > 0 || msgObj.value("msgtype").toString() == QStringLiteral("revoke")
                || msgObj.value("content").toString() == QStringLiteral("[消息已撤回]")) {
                state = QStringLiteral("deleted");
            } else if (editedAtMs > 0) {
                state = QStringLiteral("edited");
            }
            auto correctedMsg = std::make_shared<TextChatData>(msgId,
                                                               msgObj.value("content").toString(),
                                                               selfUid,
                                                               0,
                                                               senderName,
                                                               createdAt,
                                                               senderIcon,
                                                               state,
                                                               serverMsgId,
                                                               groupSeq,
                                                               replyToServerMsgId,
                                                               forwardMetaJson,
                                                               editedAtMs,
                                                               deletedAtMs);
            _gateway.userMgr()->UpsertGroupChatMsg(groupId, correctedMsg);
            _pending_group_msg_group_map.remove(msgId);
            if (selfInfo && _group_cache_store.isReady()) {
                _group_cache_store.upsertMessages(selfInfo->_uid, groupId, {correctedMsg});
            }

            int pseudoUid = 0;
            for (auto it = _group_uid_map.cbegin(); it != _group_uid_map.cend(); ++it) {
                if (it.value() == groupId) {
                    pseudoUid = it.key();
                    break;
                }
            }
            if (pseudoUid != 0) {
                const QString preview = MessageContentCodec::toPreviewText(correctedMsg->_msg_content);
                _group_list_model.updateLastMessage(pseudoUid, preview, correctedMsg->_created_at);
                _dialog_list_model.updateLastMessage(pseudoUid, preview, correctedMsg->_created_at);
            }

            if (groupId == _current_group_id) {
                _message_model.upsertMessage(correctedMsg, _gateway.userMgr()->GetUid());
                sendGroupReadAck(groupId, correctedMsg->_created_at);
            }
        }
        if (reqId == ID_FORWARD_GROUP_MSG_RSP) {
            setGroupStatus("消息已转发", false);
        }
        break;
    }
    case ID_SYNC_DRAFT_RSP: {
        const QString dialogType = payload.value("dialog_type").toString();
        int dialogUid = 0;
        if (dialogType == "group") {
            const qint64 groupId = payload.value("group_id").toVariant().toLongLong();
            if (groupId > 0) {
                dialogUid = -static_cast<int>(groupId);
            }
        } else if (dialogType == "private") {
            dialogUid = payload.value("peer_uid").toInt();
        }
        if (dialogUid != 0) {
            const QString draftText = payload.value("draft_text").toString();
            if (draftText.trimmed().isEmpty()) {
                _dialog_draft_map.remove(dialogUid);
            } else {
                _dialog_draft_map.insert(dialogUid, draftText);
            }
            const int muteState = payload.value("mute_state").toInt(
                _dialog_server_mute_map.value(dialogUid, 0));
            _dialog_server_mute_map.insert(dialogUid, muteState > 0 ? 1 : 0);
            const int idx = _dialog_list_model.indexOfUid(dialogUid);
            if (idx >= 0) {
                const QVariantMap item = _dialog_list_model.get(idx);
                _dialog_list_model.setDialogMeta(dialogUid,
                                                 item.value("dialogType").toString(),
                                                 item.value("unreadCount").toInt(),
                                                 item.value("pinnedRank").toInt(),
                                                 draftText,
                                                 item.value("lastMsgTs").toLongLong(),
                                                 muteState);
            } else {
                applyDraftToDialogModel(dialogUid, draftText);
            }
            if (dialogUid == currentDialogUid()) {
                setCurrentDraftText(draftText);
                setCurrentDialogMuted(muteState > 0);
            }
            const int ownerUid = _gateway.userMgr()->GetUid();
            if (ownerUid > 0) {
                saveDraftStore(ownerUid);
            }
        }
        break;
    }
    case ID_PIN_DIALOG_RSP: {
        const QString dialogType = payload.value("dialog_type").toString();
        int dialogUid = 0;
        if (dialogType == "group") {
            const qint64 groupId = payload.value("group_id").toVariant().toLongLong();
            if (groupId > 0) {
                dialogUid = -static_cast<int>(groupId);
            }
        } else if (dialogType == "private") {
            dialogUid = payload.value("peer_uid").toInt();
        }
        if (dialogUid != 0) {
            const int pinnedRank = qMax(0, payload.value("pinned_rank").toInt(0));
            _dialog_server_pinned_map.insert(dialogUid, pinnedRank);
            if (pinnedRank > 0) {
                _dialog_local_pinned_set.insert(dialogUid);
            } else {
                _dialog_local_pinned_set.remove(dialogUid);
            }
            if (dialogUid == currentDialogUid()) {
                setCurrentDialogPinned(pinnedRank > 0);
            }
            const int ownerUid = _gateway.userMgr()->GetUid();
            if (ownerUid > 0) {
                saveDraftStore(ownerUid);
            }
            refreshDialogModel();
        }
        break;
    }
    case ID_GET_GROUP_LIST_RSP:
    default:
        break;
    }
}

void AppController::onDialogListRsp(QJsonObject payload)
{
    if (!payload.contains("dialogs")) {
        return;
    }
    const int error = payload.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        return;
    }

    const QJsonArray dialogs = payload.value("dialogs").toArray();
    std::vector<std::shared_ptr<FriendInfo>> merged;
    merged.reserve(dialogs.size());
    _dialog_server_pinned_map.clear();
    _dialog_server_mute_map.clear();

    for (const auto &one : dialogs) {
        const QJsonObject obj = one.toObject();
        const QString dialogType = obj.value("dialog_type").toString();
        const QString title = obj.value("title").toString();
        const QString avatar = obj.value("avatar").toString();
        const QString preview = obj.value("last_msg_preview").toString();

        if (dialogType == "private") {
            const int peerUid = obj.value("peer_uid").toInt();
            if (peerUid <= 0) {
                continue;
            }
            auto item = std::make_shared<FriendInfo>(peerUid,
                                                     title,
                                                     title,
                                                     avatar,
                                                     0,
                                                     QString(),
                                                     QString(),
                                                     preview);
            item->_dialog_type = dialogType;
            item->_unread_count = obj.value("unread_count").toInt(0);
            item->_pinned_rank = obj.value("pinned_rank").toInt(0);
            _dialog_server_pinned_map.insert(peerUid, item->_pinned_rank);
            item->_draft_text = obj.value("draft_text").toString();
            qint64 lastMsgTs = obj.value("last_msg_ts").toVariant().toLongLong();
            if (lastMsgTs > 0 && lastMsgTs < 100000000000LL) {
                lastMsgTs *= 1000;
            }
            item->_last_msg_ts = lastMsgTs;
            item->_mute_state = obj.value("mute_state").toInt(0);
            _dialog_server_mute_map.insert(peerUid, item->_mute_state);
            item->_mention_count = _dialog_mention_map.value(peerUid, 0);
            if (_dialog_draft_map.contains(peerUid)) {
                item->_draft_text = _dialog_draft_map.value(peerUid);
            }
            if (_dialog_local_pinned_set.contains(peerUid)) {
                item->_pinned_rank = qMax(item->_pinned_rank, 1000000);
            }
            merged.push_back(item);
            continue;
        }

        if (dialogType == "group") {
            const qint64 groupId = obj.value("group_id").toVariant().toLongLong();
            if (groupId <= 0) {
                continue;
            }
            const int pseudoUid = -static_cast<int>(groupId);
            const QString groupIcon = avatar.trimmed().isEmpty()
                ? QStringLiteral("qrc:/res/chat_icon.png")
                : avatar;
            auto item = std::make_shared<FriendInfo>(pseudoUid,
                                                     title,
                                                     title,
                                                     groupIcon,
                                                     0,
                                                     QString(),
                                                     QString(),
                                                     preview);
            item->_dialog_type = dialogType;
            item->_unread_count = obj.value("unread_count").toInt(0);
            item->_pinned_rank = obj.value("pinned_rank").toInt(0);
            _dialog_server_pinned_map.insert(pseudoUid, item->_pinned_rank);
            item->_draft_text = obj.value("draft_text").toString();
            qint64 lastMsgTs = obj.value("last_msg_ts").toVariant().toLongLong();
            if (lastMsgTs > 0 && lastMsgTs < 100000000000LL) {
                lastMsgTs *= 1000;
            }
            item->_last_msg_ts = lastMsgTs;
            item->_mute_state = obj.value("mute_state").toInt(0);
            _dialog_server_mute_map.insert(pseudoUid, item->_mute_state);
            item->_mention_count = _dialog_mention_map.value(pseudoUid, 0);
            if (_dialog_draft_map.contains(pseudoUid)) {
                item->_draft_text = _dialog_draft_map.value(pseudoUid);
            }
            if (_dialog_local_pinned_set.contains(pseudoUid)) {
                item->_pinned_rank = qMax(item->_pinned_rank, 1000000);
            }
            merged.push_back(item);
            _group_uid_map.insert(pseudoUid, groupId);
        }
    }

    std::stable_sort(merged.begin(), merged.end(),
                     [](const std::shared_ptr<FriendInfo> &lhs, const std::shared_ptr<FriendInfo> &rhs) {
                         if (!lhs || !rhs) {
                             return static_cast<bool>(lhs);
                         }
                         if (lhs->_pinned_rank != rhs->_pinned_rank) {
                             return lhs->_pinned_rank > rhs->_pinned_rank;
                         }
                         if (lhs->_last_msg_ts != rhs->_last_msg_ts) {
                             return lhs->_last_msg_ts > rhs->_last_msg_ts;
                         }
                         return lhs->_uid < rhs->_uid;
                     });

    _dialog_list_model.setFriends(merged);
    if (_current_group_id > 0) {
        const int selectedGroupDialogUid = -static_cast<int>(_current_group_id);
        _dialog_list_model.clearUnread(selectedGroupDialogUid);
        _dialog_list_model.clearMention(selectedGroupDialogUid);
        _dialog_mention_map.remove(selectedGroupDialogUid);
    } else if (_current_chat_uid > 0) {
        _dialog_list_model.clearUnread(_current_chat_uid);
        _dialog_list_model.clearMention(_current_chat_uid);
        _dialog_mention_map.remove(_current_chat_uid);
    }
    syncCurrentDialogDraft();
}

void AppController::onPrivateHistoryRsp(QJsonObject payload)
{
    setPrivateHistoryLoading(false);
    const int error = payload.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        _private_history_pending_before_ts = 0;
        _private_history_pending_peer_uid = 0;
        setCanLoadMorePrivateHistory(false);
        return;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        _private_history_pending_before_ts = 0;
        _private_history_pending_peer_uid = 0;
        setCanLoadMorePrivateHistory(false);
        return;
    }

    const int peerUid = payload.value("peer_uid").toInt();
    const bool hasMore = payload.value("has_more").toBool(false);
    const QJsonArray messages = payload.value("messages").toArray();
    std::vector<std::shared_ptr<TextChatData>> parsed;
    parsed.reserve(messages.size());
    for (const auto &one : messages) {
        const QJsonObject obj = one.toObject();
        qint64 createdAt = obj.value("created_at").toVariant().toLongLong();
        if (createdAt <= 0) {
            createdAt = QDateTime::currentMSecsSinceEpoch();
        } else if (createdAt < 100000000000LL) {
            createdAt *= 1000;
        }
        qint64 replyToServerMsgId = obj.value("reply_to_server_msg_id").toVariant().toLongLong();
        QString forwardMetaJson;
        const QJsonValue forwardMetaValue = obj.value("forward_meta");
        if (forwardMetaValue.isObject()) {
            forwardMetaJson = QString::fromUtf8(QJsonDocument(forwardMetaValue.toObject()).toJson(QJsonDocument::Compact));
        } else if (forwardMetaValue.isArray()) {
            forwardMetaJson = QString::fromUtf8(QJsonDocument(forwardMetaValue.toArray()).toJson(QJsonDocument::Compact));
        } else if (forwardMetaValue.isString()) {
            forwardMetaJson = forwardMetaValue.toString();
        }
        qint64 editedAtMs = obj.value("edited_at_ms").toVariant().toLongLong();
        if (editedAtMs > 0 && editedAtMs < 100000000000LL) {
            editedAtMs *= 1000;
        }
        qint64 deletedAtMs = obj.value("deleted_at_ms").toVariant().toLongLong();
        if (deletedAtMs > 0 && deletedAtMs < 100000000000LL) {
            deletedAtMs *= 1000;
        }
        const QString content = obj.value("content").toString();
        const QString state = (deletedAtMs > 0 || content == QStringLiteral("[消息已撤回]"))
            ? QStringLiteral("deleted")
            : ((editedAtMs > 0) ? QStringLiteral("edited") : QStringLiteral("sent"));
        auto msg = std::make_shared<TextChatData>(
            obj.value("msgid").toString(),
            content,
            obj.value("fromuid").toInt(),
            obj.value("touid").toInt(),
            QString(),
            createdAt,
            QString(),
            state,
            0,
            0,
            replyToServerMsgId,
            forwardMetaJson,
            editedAtMs,
            deletedAtMs);
        parsed.push_back(msg);
    }
    std::sort(parsed.begin(), parsed.end(),
              [](const std::shared_ptr<TextChatData> &lhs, const std::shared_ptr<TextChatData> &rhs) {
                  if (!lhs || !rhs) {
                      return static_cast<bool>(lhs);
                  }
                  if (lhs->_created_at != rhs->_created_at) {
                      return lhs->_created_at < rhs->_created_at;
                  }
                  return lhs->_msg_id < rhs->_msg_id;
              });

    if (!parsed.empty()) {
        _private_cache_store.upsertMessages(selfInfo->_uid, peerUid, parsed);
        _gateway.userMgr()->AppendFriendChatMsg(peerUid, parsed);
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (friendInfo && !friendInfo->_chat_msgs.empty()) {
        const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
        const qint64 lastTs = friendInfo->_chat_msgs.back() ? friendInfo->_chat_msgs.back()->_created_at : 0;
        _chat_list_model.updateLastMessage(peerUid, preview, lastTs);
        _dialog_list_model.updateLastMessage(peerUid, preview, lastTs);
    }

    if (peerUid == _current_chat_uid) {
        if (_private_history_pending_before_ts > 0) {
            _message_model.prependMessages(parsed, selfInfo->_uid);
        } else if (friendInfo) {
            _message_model.setMessages(friendInfo->_chat_msgs, selfInfo->_uid);
        } else {
            _message_model.clear();
        }
        _private_history_before_ts = _message_model.earliestCreatedAt();
        setCanLoadMorePrivateHistory(hasMore);
        qint64 latestPeerTs = 0;
        if (friendInfo) {
            for (const auto &one : friendInfo->_chat_msgs) {
                if (one && one->_from_uid == peerUid) {
                    latestPeerTs = qMax(latestPeerTs, one->_created_at);
                }
            }
        }
        if (latestPeerTs > 0) {
            sendPrivateReadAck(peerUid, latestPeerTs);
        }
    }

    _private_history_pending_before_ts = 0;
    _private_history_pending_peer_uid = 0;
}

void AppController::onPrivateMsgChanged(QJsonObject payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }
    if (payload.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
        return;
    }

    const int selfUid = selfInfo->_uid;
    const int fromUid = payload.value("fromuid").toInt();
    const int peerUidField = payload.value("peer_uid").toInt();
    const QString msgId = payload.value("msgid").toString();
    const QString content = payload.value("content").toString();
    const QString event = payload.value("event").toString();
    qint64 editedAtMs = payload.value("edited_at_ms").toVariant().toLongLong();
    qint64 deletedAtMs = payload.value("deleted_at_ms").toVariant().toLongLong();
    if (editedAtMs > 0 && editedAtMs < 100000000000LL) {
        editedAtMs *= 1000;
    }
    if (deletedAtMs > 0 && deletedAtMs < 100000000000LL) {
        deletedAtMs *= 1000;
    }
    if (msgId.isEmpty() || content.isEmpty()) {
        return;
    }

    int peerUid = 0;
    if (fromUid == selfUid) {
        peerUid = peerUidField;
    } else if (fromUid > 0) {
        peerUid = fromUid;
    } else {
        peerUid = peerUidField;
    }
    if (peerUid <= 0) {
        return;
    }

    const QString state = (event == "private_msg_revoked")
        ? QStringLiteral("deleted")
        : QStringLiteral("edited");
    if (!_gateway.userMgr()->UpdatePrivateChatMsgContent(peerUid, msgId, content, state, editedAtMs, deletedAtMs)) {
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (!friendInfo) {
        return;
    }

    if (_private_cache_store.isReady()) {
        _private_cache_store.upsertMessages(selfUid, peerUid, friendInfo->_chat_msgs);
    }
    if (!friendInfo->_chat_msgs.empty() && friendInfo->_chat_msgs.back()) {
        const QString preview = MessageContentCodec::toPreviewText(friendInfo->_chat_msgs.back()->_msg_content);
        const qint64 lastTs = friendInfo->_chat_msgs.back()->_created_at;
        _chat_list_model.updateLastMessage(peerUid, preview, lastTs);
        _dialog_list_model.updateLastMessage(peerUid, preview, lastTs);
    }
    if (peerUid == _current_chat_uid) {
        _message_model.setMessages(friendInfo->_chat_msgs, selfUid);
    }
}

void AppController::onPrivateReadAck(QJsonObject payload)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }
    if (payload.value("error").toInt(ErrorCodes::ERR_JSON) != ErrorCodes::SUCCESS) {
        return;
    }

    const int selfUid = selfInfo->_uid;
    const int readerUid = payload.value("fromuid").toInt();
    if (readerUid <= 0 || readerUid == selfUid) {
        return;
    }

    qint64 readTs = payload.value("read_ts").toVariant().toLongLong();
    if (readTs <= 0) {
        readTs = QDateTime::currentMSecsSinceEpoch();
    } else if (readTs < 100000000000LL) {
        readTs *= 1000;
    }
    if (_gateway.userMgr()->MarkPrivateOutgoingReadUntil(readerUid, selfUid, readTs) <= 0) {
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(readerUid);
    if (!friendInfo) {
        return;
    }
    if (_private_cache_store.isReady()) {
        _private_cache_store.upsertMessages(selfUid, readerUid, friendInfo->_chat_msgs);
    }
    if (readerUid == _current_chat_uid) {
        _message_model.setMessages(friendInfo->_chat_msgs, selfUid);
    }
}

void AppController::refreshFriendModels()
{
    _chat_list_model.clear();
    const auto chatList = _gateway.userMgr()->GetChatListPerPage();
    _chat_list_model.setFriends(chatList);
    _gateway.userMgr()->UpdateChatLoadedCount();
    refreshChatLoadMoreState();

    _contact_list_model.clear();
    const auto contactList = _gateway.userMgr()->GetConListPerPage();
    _contact_list_model.setFriends(contactList);
    _gateway.userMgr()->UpdateContactLoadedCount();
    refreshContactLoadMoreState();

    refreshGroupModel();
    refreshDialogModel();
}

void AppController::refreshApplyModel()
{
    const auto applyList = _gateway.userMgr()->GetApplyListSnapshot();
    _apply_request_model.setApplies(applyList);
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

    const auto chats = _gateway.userMgr()->GetFriendListSnapshot();
    dialogs.reserve(chats.size() + _group_uid_map.size());
    for (const auto &chat : chats) {
        if (!chat) {
            continue;
        }
        auto one = std::make_shared<FriendInfo>(chat->_uid,
                                                chat->_name,
                                                chat->_nick,
                                                chat->_icon,
                                                chat->_sex,
                                                chat->_desc,
                                                chat->_back,
                                                chat->_last_msg,
                                                chat->_user_id);
        one->_dialog_type = QStringLiteral("private");
        one->_pinned_rank = qMax(one->_pinned_rank, _dialog_server_pinned_map.value(chat->_uid, 0));
        one->_mute_state = _dialog_server_mute_map.value(chat->_uid, one->_mute_state);
        one->_mention_count = _dialog_mention_map.value(chat->_uid, 0);
        if (!chat->_chat_msgs.empty() && chat->_chat_msgs.back()) {
            one->_last_msg_ts = chat->_chat_msgs.back()->_created_at;
        }
        if (_dialog_draft_map.contains(chat->_uid)) {
            one->_draft_text = _dialog_draft_map.value(chat->_uid);
        }
        if (_dialog_local_pinned_set.contains(chat->_uid)) {
            one->_pinned_rank = qMax(one->_pinned_rank, 1000000);
        }
        dialogs.push_back(one);
    }

    const auto groups = _gateway.userMgr()->GetGroupListSnapshot();
    for (const auto &group : groups) {
        if (!group || group->_group_id <= 0) {
            continue;
        }
        const int pseudoUid = -static_cast<int>(group->_group_id);
        const QString groupIcon = group->_icon.trimmed().isEmpty()
            ? QStringLiteral("qrc:/res/chat_icon.png")
            : group->_icon;
        auto one = std::make_shared<FriendInfo>(pseudoUid,
                                                group->_name,
                                                group->_name,
                                                groupIcon,
                                                0,
                                                group->_announcement,
                                                group->_announcement,
                                                group->_last_msg);
        one->_dialog_type = QStringLiteral("group");
        one->_pinned_rank = qMax(one->_pinned_rank, _dialog_server_pinned_map.value(pseudoUid, 0));
        one->_mute_state = _dialog_server_mute_map.value(pseudoUid, one->_mute_state);
        one->_mention_count = _dialog_mention_map.value(pseudoUid, 0);
        if (!group->_chat_msgs.empty() && group->_chat_msgs.back()) {
            one->_last_msg_ts = group->_chat_msgs.back()->_created_at;
        }
        if (_dialog_draft_map.contains(pseudoUid)) {
            one->_draft_text = _dialog_draft_map.value(pseudoUid);
        }
        if (_dialog_local_pinned_set.contains(pseudoUid)) {
            one->_pinned_rank = qMax(one->_pinned_rank, 1000000);
        }
        dialogs.push_back(one);
    }

    std::stable_sort(dialogs.begin(), dialogs.end(),
                     [](const std::shared_ptr<FriendInfo> &lhs, const std::shared_ptr<FriendInfo> &rhs) {
                         if (!lhs || !rhs) {
                             return static_cast<bool>(lhs);
                         }
                         if (lhs->_pinned_rank != rhs->_pinned_rank) {
                             return lhs->_pinned_rank > rhs->_pinned_rank;
                         }
                         if (lhs->_last_msg_ts != rhs->_last_msg_ts) {
                             return lhs->_last_msg_ts > rhs->_last_msg_ts;
                         }
                         return lhs->_uid < rhs->_uid;
                     });

    _dialog_list_model.setFriends(dialogs);
    syncCurrentDialogDraft();
}

void AppController::loadCurrentChatMessages()
{
    if (_current_chat_uid <= 0) {
        _message_model.clear();
        _private_history_before_ts = 0;
        _private_history_pending_before_ts = 0;
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
        _private_history_pending_before_ts = 0;
        _private_history_pending_peer_uid = 0;
        _group_history_before_seq = 0;
        _group_history_has_more = true;
        setPrivateHistoryLoading(false);
        setCanLoadMorePrivateHistory(false);
        return;
    }

    setCurrentChatPeerName(friendInfo->_name);
    setCurrentChatPeerIcon(friendInfo->_icon);

    const auto localMessages = _private_cache_store.loadRecentMessages(selfInfo->_uid, _current_chat_uid, 20);
    if (!localMessages.empty()) {
        _gateway.userMgr()->AppendFriendChatMsg(_current_chat_uid, localMessages);
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
    _private_history_pending_before_ts = 0;
    _private_history_pending_peer_uid = 0;
    setPrivateHistoryLoading(false);
    setCanLoadMorePrivateHistory(true);
    requestPrivateHistory(_current_chat_uid, 0);
}

void AppController::requestPrivateHistory(int peerUid, qint64 beforeTs)
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
    payloadObj["limit"] = 20;
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);

    _private_history_pending_peer_uid = peerUid;
    _private_history_pending_before_ts = beforeTs;
    setPrivateHistoryLoading(true);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_PRIVATE_HISTORY_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_GROUP_READ_ACK_REQ, payload);
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
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_PRIVATE_READ_ACK_REQ, payload);
}

void AppController::setContactPane(ContactPane pane)
{
    if (_contact_pane == pane) {
        return;
    }

    _contact_pane = pane;
    emit contactPaneChanged();
}

void AppController::setCurrentContact(int uid, const QString &name, const QString &nick, const QString &icon,
                                      const QString &back, int sex, const QString &userId)
{
    const QString normalizedIcon = normalizeIconPath(icon);
    if (_current_contact_uid == uid
        && _current_contact_name == name
        && _current_contact_nick == nick
        && _current_contact_icon == normalizedIcon
        && _current_contact_back == back
        && _current_contact_sex == sex
        && _current_contact_user_id == userId) {
        return;
    }

    _current_contact_uid = uid;
    _current_contact_name = name;
    _current_contact_nick = nick;
    _current_contact_icon = normalizedIcon;
    _current_contact_back = back;
    _current_contact_sex = sex;
    _current_contact_user_id = userId;
    emit currentContactChanged();
}

void AppController::setCurrentChatPeerName(const QString &name)
{
    if (_current_chat_peer_name == name) {
        return;
    }

    _current_chat_peer_name = name;
    emit currentChatPeerChanged();
}

void AppController::setCurrentChatPeerIcon(const QString &icon)
{
    const QString normalizedIcon = normalizeIconPath(icon);
    if (_current_chat_peer_icon == normalizedIcon) {
        return;
    }

    _current_chat_peer_icon = normalizedIcon;
    emit currentChatPeerChanged();
}

void AppController::selectChatByUid(int uid)
{
    const int idx = _chat_list_model.indexOfUid(uid);
    if (idx >= 0) {
        selectChatIndex(idx);
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(uid);
    if (!friendInfo) {
        return;
    }

    _current_chat_uid = uid;
    setCurrentGroup(0, "");
    setCurrentChatPeerName(friendInfo->_name);
    setCurrentChatPeerIcon(friendInfo->_icon);
    loadCurrentChatMessages();
    syncCurrentDialogDraft();
}

void AppController::setSearchPending(bool pending)
{
    if (_search_pending == pending) {
        return;
    }

    _search_pending = pending;
    emit searchPendingChanged();
}

void AppController::setSearchStatus(const QString &text, bool isError)
{
    if (_search_status_text == text && _search_status_error == isError) {
        return;
    }

    _search_status_text = text;
    _search_status_error = isError;
    emit searchStatusChanged();
}

void AppController::clearSearchResultOnly()
{
    _search_result_model.clear();
}

void AppController::setChatLoadingMore(bool loading)
{
    if (_chat_loading_more == loading) {
        return;
    }

    _chat_loading_more = loading;
    emit chatLoadingMoreChanged();
}

void AppController::setPrivateHistoryLoading(bool loading)
{
    if (_private_history_loading == loading) {
        return;
    }
    _private_history_loading = loading;
    emit privateHistoryLoadingChanged();
}

void AppController::setCanLoadMorePrivateHistory(bool canLoad)
{
    if (_can_load_more_private_history == canLoad) {
        return;
    }
    _can_load_more_private_history = canLoad;
    emit canLoadMorePrivateHistoryChanged();
}

void AppController::setContactLoadingMore(bool loading)
{
    if (_contact_loading_more == loading) {
        return;
    }

    _contact_loading_more = loading;
    emit contactLoadingMoreChanged();
}

void AppController::setAuthStatus(const QString &text, bool isError)
{
    if (_auth_status_text == text && _auth_status_error == isError) {
        return;
    }

    _auth_status_text = text;
    _auth_status_error = isError;
    emit authStatusChanged();
}

void AppController::setSettingsStatus(const QString &text, bool isError)
{
    if (_settings_status_text == text && _settings_status_error == isError) {
        return;
    }

    _settings_status_text = text;
    _settings_status_error = isError;
    emit settingsStatusChanged();
}

void AppController::setCurrentGroup(qint64 groupId, const QString &name, const QString &groupCode)
{
    const QString normalizedName = (groupId > 0) ? name : QString();
    const QString normalizedCode = (groupId > 0) ? groupCode : QString();
    if (_current_group_id == groupId && _current_group_name == normalizedName && _current_group_code == normalizedCode) {
        return;
    }

    _current_group_id = groupId;
    _current_group_name = normalizedName;
    _current_group_code = normalizedCode;
    emit currentGroupChanged();
}

void AppController::setGroupStatus(const QString &text, bool isError)
{
    if (_group_status_text == text && _group_status_error == isError) {
        return;
    }

    _group_status_text = text;
    _group_status_error = isError;
    emit groupStatusChanged();
}

void AppController::setCurrentDraftText(const QString &text)
{
    if (_current_draft_text == text) {
        return;
    }
    _current_draft_text = text;
    emit currentDraftTextChanged();
}

void AppController::setCurrentDialogPinned(bool pinned)
{
    if (_current_dialog_pinned == pinned) {
        return;
    }
    _current_dialog_pinned = pinned;
    emit currentDialogPinnedChanged();
}

void AppController::setCurrentDialogMuted(bool muted)
{
    if (_current_dialog_muted == muted) {
        return;
    }
    _current_dialog_muted = muted;
    emit currentDialogMutedChanged();
}

void AppController::setPendingReplyContext(const QString &msgId, const QString &senderName, const QString &previewText)
{
    const QString normalizedMsgId = msgId.trimmed();
    const QString normalizedSender = senderName.trimmed();
    QString normalizedPreview = previewText.trimmed();
    if (normalizedPreview.length() > 120) {
        normalizedPreview = normalizedPreview.left(120);
    }
    if (_reply_to_msg_id == normalizedMsgId
        && _reply_target_name == normalizedSender
        && _reply_preview_text == normalizedPreview) {
        return;
    }
    _reply_to_msg_id = normalizedMsgId;
    _reply_target_name = normalizedSender;
    _reply_preview_text = normalizedPreview;
    emit pendingReplyChanged();
}

int AppController::currentDialogUid() const
{
    if (_current_group_id > 0) {
        return -static_cast<int>(_current_group_id);
    }
    if (_current_chat_uid > 0) {
        return _current_chat_uid;
    }
    return 0;
}

bool AppController::resolveDialogTarget(int dialogUid, QString &dialogType, int &peerUid, qint64 &groupId) const
{
    dialogType.clear();
    peerUid = 0;
    groupId = 0;
    if (dialogUid == 0) {
        return false;
    }
    if (dialogUid > 0) {
        dialogType = "private";
        peerUid = dialogUid;
        return true;
    }

    const qint64 candidateGroupId = -static_cast<qint64>(dialogUid);
    if (candidateGroupId <= 0) {
        return false;
    }
    dialogType = "group";
    groupId = candidateGroupId;
    return true;
}

qint64 AppController::currentGroupPermissionBitsRaw() const
{
    if (_current_group_id <= 0) {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
    if (!groupInfo) {
        return 0;
    }
    if (groupInfo->_role >= 3) {
        return kOwnerPermBits;
    }
    if (groupInfo->_role < 2) {
        return 0;
    }
    if (groupInfo->_permission_bits <= 0) {
        return kDefaultAdminPermBits;
    }
    return groupInfo->_permission_bits;
}

bool AppController::hasCurrentGroupPermission(qint64 permissionBit) const
{
    if (permissionBit <= 0) {
        return false;
    }
    return (currentGroupPermissionBitsRaw() & permissionBit) != 0;
}

qint64 AppController::latestGroupCreatedAt(qint64 groupId) const
{
    if (groupId <= 0) {
        return 0;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo) {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto &one : groupInfo->_chat_msgs) {
        if (one) {
            latestTs = qMax(latestTs, one->_created_at);
        }
    }
    return latestTs;
}

qint64 AppController::latestPrivatePeerCreatedAt(int peerUid) const
{
    if (peerUid <= 0) {
        return 0;
    }
    auto friendInfo = _gateway.userMgr()->GetFriendById(peerUid);
    if (!friendInfo) {
        return 0;
    }
    qint64 latestTs = 0;
    for (const auto &one : friendInfo->_chat_msgs) {
        if (one && one->_from_uid == peerUid) {
            latestTs = qMax(latestTs, one->_created_at);
        }
    }
    return latestTs;
}

void AppController::syncCurrentDialogDraft()
{
    const int dialogUid = currentDialogUid();
    if (dialogUid == 0) {
        setCurrentDraftText("");
        setCurrentDialogPinned(false);
        setCurrentDialogMuted(false);
        setPendingReplyContext(QString(), QString(), QString());
        return;
    }
    setCurrentDraftText(_dialog_draft_map.value(dialogUid));
    const bool pinned = _dialog_local_pinned_set.contains(dialogUid)
        || _dialog_server_pinned_map.value(dialogUid, 0) > 0;
    setCurrentDialogPinned(pinned);
    setCurrentDialogMuted(_dialog_server_mute_map.value(dialogUid, 0) > 0);
}

void AppController::loadDraftStore(int ownerUid)
{
    _dialog_draft_map.clear();
    _dialog_local_pinned_set.clear();
    _dialog_server_pinned_map.clear();
    _dialog_server_mute_map.clear();
    _dialog_mention_map.clear();
    if (ownerUid <= 0) {
        return;
    }

    QSettings settings("MemoChat", "MemoChatQml");
    const QVariantMap serialized = settings.value(QString("chat_drafts/%1").arg(ownerUid)).toMap();
    for (auto it = serialized.cbegin(); it != serialized.cend(); ++it) {
        bool ok = false;
        const int dialogUid = it.key().toInt(&ok);
        if (!ok) {
            continue;
        }
        const QString draftText = it.value().toString();
        if (draftText.trimmed().isEmpty()) {
            continue;
        }
        _dialog_draft_map.insert(dialogUid, draftText);
    }

    const QStringList pinnedList = settings.value(QString("chat_pinned/%1").arg(ownerUid)).toStringList();
    for (const QString &entry : pinnedList) {
        bool ok = false;
        const int dialogUid = entry.toInt(&ok);
        if (ok && dialogUid != 0) {
            _dialog_local_pinned_set.insert(dialogUid);
        }
    }
}

void AppController::saveDraftStore(int ownerUid) const
{
    if (ownerUid <= 0) {
        return;
    }

    QVariantMap serialized;
    for (auto it = _dialog_draft_map.cbegin(); it != _dialog_draft_map.cend(); ++it) {
        if (it.value().trimmed().isEmpty()) {
            continue;
        }
        serialized.insert(QString::number(it.key()), it.value());
    }

    QSettings settings("MemoChat", "MemoChatQml");
    settings.setValue(QString("chat_drafts/%1").arg(ownerUid), serialized);
    QStringList pinnedList;
    pinnedList.reserve(_dialog_local_pinned_set.size());
    for (int uid : _dialog_local_pinned_set) {
        pinnedList.push_back(QString::number(uid));
    }
    settings.setValue(QString("chat_pinned/%1").arg(ownerUid), pinnedList);
}

void AppController::applyDraftToDialogModel(int dialogUid, const QString &draftText)
{
    const int idx = _dialog_list_model.indexOfUid(dialogUid);
    if (idx < 0) {
        return;
    }
    const QVariantMap item = _dialog_list_model.get(idx);
    _dialog_list_model.setDialogMeta(dialogUid,
                                     item.value("dialogType").toString(),
                                     item.value("unreadCount").toInt(),
                                     item.value("pinnedRank").toInt(),
                                     draftText,
                                     item.value("lastMsgTs").toLongLong(),
                                     item.value("muteState").toInt());
}

void AppController::refreshChatLoadMoreState()
{
    const bool canLoad = !_gateway.userMgr()->IsLoadChatFin();
    if (_can_load_more_chats == canLoad) {
        return;
    }

    _can_load_more_chats = canLoad;
    emit canLoadMoreChatsChanged();
}

void AppController::refreshContactLoadMoreState()
{
    const bool canLoad = !_gateway.userMgr()->IsLoadConFin();
    if (_can_load_more_contacts == canLoad) {
        return;
    }

    _can_load_more_contacts = canLoad;
    emit canLoadMoreContactsChanged();
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

bool AppController::dispatchChatContent(const QString &content, const QString &previewText)
{
    if (_current_chat_uid <= 0) {
        return false;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return false;
    }

    OutgoingChatPacket packet;
    packet.fromUid = selfInfo->_uid;
    packet.toUid = _current_chat_uid;
    packet.msgId = QUuid::createUuid().toString();
    packet.content = content;
    packet.createdAt = QDateTime::currentMSecsSinceEpoch();

    QString errorText;
    if (!_chat_controller.dispatchChatText(packet, &errorText)) {
        if (!errorText.isEmpty()) {
            setTip(errorText, true);
        }
        return false;
    }

    auto msg = std::make_shared<TextChatData>(packet.msgId, content, packet.fromUid, packet.toUid, QString(), packet.createdAt);
    _gateway.userMgr()->AppendFriendChatMsg(_current_chat_uid, {msg});
    _private_cache_store.upsertMessages(packet.fromUid, _current_chat_uid, {msg});
    _message_model.appendMessage(msg, packet.fromUid);
    _private_history_before_ts = _message_model.earliestCreatedAt();

    const QString resolvedPreview = previewText.isEmpty() ? MessageContentCodec::toPreviewText(content) : previewText;
    _chat_list_model.updateLastMessage(_current_chat_uid, resolvedPreview, packet.createdAt);
    _dialog_list_model.updateLastMessage(_current_chat_uid, resolvedPreview, packet.createdAt);
    _dialog_list_model.clearUnread(_current_chat_uid);
    return true;
}

bool AppController::dispatchGroupChatContent(const QString &content, const QString &previewText)
{
    if (_current_group_id <= 0) {
        return false;
    }

    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return false;
    }

    const QString msgId = QUuid::createUuid().toString();
    const qint64 createdAt = QDateTime::currentMSecsSinceEpoch();
    const DecodedMessageContent decoded = MessageContentCodec::decode(content);
    const QString plainText = decoded.content;
    const bool mentionAll = plainText.contains("@all", Qt::CaseInsensitive)
        || plainText.contains("@所有人");
    QJsonArray mentions;
    const QRegularExpression mentionIdRegex("@u([1-9][0-9]{8})");
    QRegularExpressionMatchIterator mentionIt = mentionIdRegex.globalMatch(plainText);
    while (mentionIt.hasNext()) {
        const QRegularExpressionMatch m = mentionIt.next();
        bool ok = false;
        const int uid = m.captured(1).toInt(&ok);
        if (ok && uid > 0) {
            mentions.append(uid);
        }
    }
    qint64 replyToServerMsgId = 0;
    if (!_reply_to_msg_id.isEmpty()) {
        auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
        if (groupInfo) {
            for (const auto &one : groupInfo->_chat_msgs) {
                if (!one || one->_msg_id != _reply_to_msg_id) {
                    continue;
                }
                if (one->_server_msg_id > 0) {
                    replyToServerMsgId = one->_server_msg_id;
                }
                break;
            }
        }
    }

    QJsonObject msgObj;
    msgObj["msgid"] = msgId;
    msgObj["content"] = content;
    msgObj["msgtype"] = decoded.type.isEmpty() ? "text" : decoded.type;
    msgObj["mentions"] = mentions;
    msgObj["mention_all"] = mentionAll;
    if (replyToServerMsgId > 0) {
        msgObj["reply_to_server_msg_id"] = replyToServerMsgId;
    }
    if (!decoded.fileName.isEmpty()) {
        msgObj["file_name"] = decoded.fileName;
    }

    QJsonObject payloadObj;
    payloadObj["fromuid"] = selfInfo->_uid;
    payloadObj["groupid"] = static_cast<qint64>(_current_group_id);
    payloadObj["msg"] = msgObj;
    const QByteArray payload = QJsonDocument(payloadObj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_GROUP_CHAT_MSG_REQ, payload);

    const QString senderName = selfInfo->_nick.isEmpty() ? selfInfo->_name : selfInfo->_nick;
    auto msg = std::make_shared<TextChatData>(msgId,
                                              content,
                                              selfInfo->_uid,
                                              0,
                                              senderName,
                                              createdAt,
                                              _current_user_icon,
                                              "sending",
                                              0,
                                              0,
                                              replyToServerMsgId);
    _gateway.userMgr()->UpsertGroupChatMsg(_current_group_id, msg);
    _pending_group_msg_group_map.insert(msgId, _current_group_id);
    if (_group_cache_store.isReady()) {
        _group_cache_store.upsertMessages(selfInfo->_uid, _current_group_id, {msg});
    }
    _message_model.appendMessage(msg, selfInfo->_uid);

    int pseudoUid = 0;
    for (auto it = _group_uid_map.cbegin(); it != _group_uid_map.cend(); ++it) {
        if (it.value() == _current_group_id) {
            pseudoUid = it.key();
            break;
        }
    }

    if (pseudoUid != 0) {
        const QString resolvedPreview = previewText.isEmpty() ? MessageContentCodec::toPreviewText(content) : previewText;
        _group_list_model.updateLastMessage(pseudoUid, resolvedPreview, createdAt);
        _dialog_list_model.updateLastMessage(pseudoUid, resolvedPreview, createdAt);
        _dialog_list_model.clearUnread(pseudoUid);
    }

    return true;
}

bool AppController::ensureCallTargetFromCurrentChat()
{
    if (hasCurrentContact()) {
        return true;
    }
    if (_current_group_id > 0) {
        return false;
    }
    if (_current_chat_uid <= 0) {
        return false;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
    if (!friendInfo) {
        return false;
    }

    setCurrentContact(friendInfo->_uid,
                      friendInfo->_name,
                      friendInfo->_nick,
                      friendInfo->_icon,
                      friendInfo->_back,
                      friendInfo->_sex,
                      friendInfo->_user_id);
    return hasCurrentContact();
}

void AppController::sendCallInvite(const QString &callType)
{
    if (_current_contact_uid <= 0) {
        setAuthStatus("请选择联系人", true);
        return;
    }

    if (_current_chat_uid != _current_contact_uid) {
        selectChatByUid(_current_contact_uid);
    }
    if (_current_chat_uid != _current_contact_uid) {
        _current_chat_uid = _current_contact_uid;
        setCurrentChatPeerName(_current_contact_name);
        setCurrentChatPeerIcon(_current_contact_icon);
        loadCurrentChatMessages();
    }

    const QString joinUrl = buildCallJoinUrl(callType);
    const QString encoded = MessageContentCodec::encodeCallInvite(callType, joinUrl);
    const QString preview = (callType == "video") ? "[视频通话邀请]" : "[语音通话邀请]";
    if (!dispatchChatContent(encoded, preview)) {
        setAuthStatus("通话邀请发送失败", true);
        return;
    }

    QString errorText;
    if (!LocalFilePickerService::openUrl(joinUrl, &errorText)) {
        setAuthStatus(errorText.isEmpty() ? "打开通话链接失败" : errorText, true);
        return;
    }

    setAuthStatus("通话邀请已发送", false);
}

QString AppController::buildCallJoinUrl(const QString &callType) const
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return {};
    }
    return _chat_controller.buildCallJoinUrl(selfInfo->_uid, _current_contact_uid, callType);
}

void AppController::setTip(const QString &tip, bool isError)
{
    if (_tip_text == tip && _tip_error == isError) {
        return;
    }
    _tip_text = tip;
    _tip_error = isError;
    emit tipChanged();
}

void AppController::setBusy(bool value)
{
    if (_busy == value) {
        return;
    }
    _busy = value;
    emit busyChanged();
}

void AppController::setPage(Page newPage)
{
    if (_page == newPage) {
        return;
    }
    _page = newPage;
    emit pageChanged();
}

QString AppController::normalizeIconPath(QString icon) const
{
    return normalizeIconForQml(icon);
}


#include "AppController.h"
#include "LocalFilePickerService.h"
#include "MediaUploadService.h"
#include "MessageContentCodec.h"
#include "IconPathUtils.h"
#include "httpmgr.h"
#include "tcpmgr.h"
#include "usermgr.h"
#include <QFileInfo>
#include <QUuid>

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
      _contact_list_model(this),
      _message_model(this),
      _search_result_model(this),
      _apply_request_model(this),
      _search_pending(false),
      _search_status_error(false),
      _chat_loading_more(false),
      _can_load_more_chats(false),
      _contact_loading_more(false),
      _can_load_more_contacts(false),
      _auth_status_error(false),
      _settings_status_error(false),
      _group_status_error(false),
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
    connect(&_apply_request_model, &ApplyRequestModel::unapprovedCountChanged,
            this, &AppController::pendingApplyChanged);

    connect(&_register_countdown_timer, &QTimer::timeout,
            this, &AppController::onRegisterCountdownTimeout);
    connect(&_heartbeat_timer, &QTimer::timeout,
            this, &AppController::onHeartbeatTimeout);
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

QString AppController::currentGroupName() const
{
    return _current_group_name;
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

void AppController::switchToLogin()
{
    _register_countdown_timer.stop();
    _heartbeat_timer.stop();
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
    _contact_list_model.clear();
    _message_model.clear();
    _search_result_model.clear();
    _apply_request_model.clear();
    setSearchPending(false);
    setSearchStatus("", false);
    setChatLoadingMore(false);
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
    emit currentGroupChanged();
    _group_uid_map.clear();
    setCurrentChatPeerName("");
    setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
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
        _current_chat_uid = 0;
        setCurrentGroup(0, "");
        setCurrentChatPeerName("");
        setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
        _message_model.clear();
        return;
    }

    _current_chat_uid = item.value("uid").toInt();
    setCurrentGroup(0, "");
    setCurrentChatPeerName(item.value("name").toString());
    setCurrentChatPeerIcon(item.value("icon").toString());
    loadCurrentChatMessages();
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
                      item.value("sex").toInt());
    setContactPane(FriendInfoPane);
    setAuthStatus("", false);
}

void AppController::selectGroupIndex(int index)
{
    const QVariantMap item = _group_list_model.get(index);
    if (item.isEmpty()) {
        setCurrentGroup(0, "");
        _message_model.clear();
        return;
    }

    const int pseudoUid = item.value("uid").toInt();
    const qint64 groupId = _group_uid_map.value(pseudoUid, 0);
    if (groupId <= 0) {
        setCurrentGroup(0, "");
        _message_model.clear();
        return;
    }

    setCurrentGroup(groupId, item.value("name").toString());
    _current_chat_uid = 0;
    setCurrentChatPeerName(_current_group_name);
    setCurrentChatPeerIcon("qrc:/res/chat_icon.png");

    auto groupInfo = _gateway.userMgr()->GetGroupById(groupId);
    if (!groupInfo) {
        _message_model.clear();
        return;
    }

    _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
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

    const QString content = text;
    if (content.trimmed().isEmpty()) {
        return;
    }

    if (content.size() > 1024) {
        setTip("单条消息不能超过1024字符", true);
        return;
    }

    if (_current_group_id > 0) {
        if (!dispatchGroupChatContent(content, content)) {
            setTip("群消息发送失败", true);
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
    if (!hasCurrentContact()) {
        return;
    }
    sendCallInvite("voice");
}

void AppController::startVideoChat()
{
    if (!hasCurrentContact()) {
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

    _profile_controller.sendSaveProfile(
        selfInfo->_uid, selfInfo->_name, nick, desc, _current_user_icon);
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

void AppController::createGroup(const QString &name, const QVariantList &memberUidList)
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

    QJsonArray members;
    for (const auto &one : memberUidList) {
        const int uid = one.toInt();
        if (uid > 0 && uid != selfInfo->_uid) {
            members.append(uid);
        }
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["name"] = trimmedName;
    obj["member_limit"] = 200;
    obj["members"] = members;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_CREATE_GROUP_REQ, payload);
    setGroupStatus("正在创建群聊...", false);
}

void AppController::inviteGroupMember(int uid, const QString &reason)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0 || uid <= 0) {
        setGroupStatus("邀请参数非法", true);
        return;
    }
    if (!_gateway.userMgr()->CheckFriendById(uid)) {
        setGroupStatus("仅支持邀请好友入群", true);
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["touid"] = uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["reason"] = reason;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_INVITE_GROUP_MEMBER_REQ, payload);
    setGroupStatus("邀请已发送", false);
}

void AppController::applyJoinGroup(qint64 groupId, const QString &reason)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || groupId <= 0) {
        setGroupStatus("申请参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = groupId;
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

void AppController::loadGroupHistory()
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0) {
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["before_ts"] = 0;
    obj["limit"] = 50;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_GROUP_HISTORY_REQ, payload);
}

void AppController::updateGroupAnnouncement(const QString &announcement)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0) {
        setGroupStatus("请选择群聊", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["announcement"] = announcement.left(1000);
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_UPDATE_GROUP_ANNOUNCEMENT_REQ, payload);
}

void AppController::setGroupAdmin(int uid, bool isAdmin)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0 || uid <= 0) {
        setGroupStatus("设置管理员参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["touid"] = uid;
    obj["is_admin"] = isAdmin;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_SET_GROUP_ADMIN_REQ, payload);
}

void AppController::muteGroupMember(int uid, int muteSeconds)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0 || uid <= 0) {
        setGroupStatus("禁言参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["touid"] = uid;
    obj["mute_seconds"] = qMax(0, muteSeconds);
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_MUTE_GROUP_MEMBER_REQ, payload);
}

void AppController::kickGroupMember(int uid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _current_group_id <= 0 || uid <= 0) {
        setGroupStatus("踢人参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_current_group_id);
    obj["touid"] = uid;
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
        setTip("参数错误", true);
        return;
    }

    ServerInfo server_info;
    server_info.Uid = obj.value("uid").toInt();
    server_info.Host = obj.value("host").toString();
    server_info.Port = obj.value("port").toString();
    server_info.Token = obj.value("token").toString();

    _pending_uid = server_info.Uid;
    _pending_token = server_info.Token;
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
        setBusy(false);
        setTip("网络异常", true);
        return;
    }

    setTip("聊天服务连接成功，正在登录...", false);
    QJsonObject obj;
    obj["uid"] = _pending_uid;
    obj["token"] = _pending_token;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.tcpMgr()->slot_send_data(ReqId::ID_CHAT_LOGIN, payload);
}

void AppController::onChatLoginFailed(int err)
{
    setBusy(false);
    setTip(QString("登录失败, err is %1").arg(err), true);
}

void AppController::onSwitchToChat()
{
    setBusy(false);
    setTip("", false);
    setSearchPending(false);
    setChatLoadingMore(false);
    setContactLoadingMore(false);
    setAuthStatus("", false);
    setSettingsStatus("", false);
    setContactPane(ApplyRequestPane);
    setCurrentContact(0, "", "", "qrc:/res/head_1.jpg", "", 0);
    setPage(ChatPage);

    auto user_info = _gateway.userMgr()->GetUserInfo();
    if (user_info) {
        const QString name = user_info->_name;
        const QString nick = user_info->_nick;
        const QString icon = normalizeIconPath(user_info->_icon);
        const QString desc = user_info->_desc;

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
        emit currentUserChanged();
    }

    refreshFriendModels();
    refreshApplyModel();
    refreshGroupList();
    if (_chat_list_model.count() > 0) {
        selectChatIndex(0);
    } else {
        _current_chat_uid = 0;
        setCurrentChatPeerName("");
        setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
    }

    // Keep chat session alive; server considers session expired after heartbeat timeout.
    _heartbeat_timer.start(10000);
    onHeartbeatTimeout();
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
    refreshChatLoadMoreState();
    refreshContactLoadMoreState();
    if (authInfo) {
        _gateway.userMgr()->MarkApplyStatus(authInfo->_uid, 1);
        _apply_request_model.markApproved(authInfo->_uid);
        if (_current_contact_uid == authInfo->_uid) {
            setCurrentContact(authInfo->_uid, authInfo->_name, authInfo->_nick, authInfo->_icon,
                              authInfo->_nick, authInfo->_sex);
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
    refreshChatLoadMoreState();
    refreshContactLoadMoreState();
    if (authRsp) {
        _gateway.userMgr()->MarkApplyStatus(authRsp->_uid, 1);
        _apply_request_model.markApproved(authRsp->_uid);
        setAuthStatus("好友添加成功", false);
        if (_current_contact_uid == authRsp->_uid) {
            setCurrentContact(authRsp->_uid, authRsp->_name, authRsp->_nick, authRsp->_icon,
                              authRsp->_nick, authRsp->_sex);
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
    const int peerUid = (msg->_from_uid == selfUid) ? msg->_to_uid : msg->_from_uid;
    _gateway.userMgr()->AppendFriendChatMsg(peerUid, msg->_chat_msgs);
    const QString preview = MessageContentCodec::toPreviewText(msg->_chat_msgs.front()->_msg_content);
    _chat_list_model.updateLastMessage(peerUid, preview);

    if (peerUid != _current_chat_uid) {
        return;
    }

    for (const auto &chat : msg->_chat_msgs) {
        _message_model.appendMessage(chat, selfUid);
    }
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
    if (_page != ChatPage) {
        return;
    }

    _heartbeat_timer.stop();
    switchToLogin();
    setTip("心跳超时或连接断开，请重新登录", true);
}

void AppController::onGroupListUpdated()
{
    refreshGroupModel();
    if (_current_group_id > 0) {
        auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
        if (groupInfo) {
            _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
            return;
        }

        setCurrentGroup(0, "");
        if (_current_chat_uid > 0) {
            loadCurrentChatMessages();
        } else {
            _message_model.clear();
            setCurrentChatPeerName("");
            setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
        }
    }
}

void AppController::onGroupInvite(qint64 groupId, QString groupName, int operatorUid)
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
        setCurrentGroup(groupId, groupName);
    }
}

void AppController::onGroupApply(qint64 groupId, int applicantUid, QString reason)
{
    Q_UNUSED(groupId);
    setGroupStatus(QString("收到入群申请：UID %1 %2").arg(applicantUid).arg(reason), false);
}

void AppController::onGroupMemberChanged(QJsonObject payload)
{
    const QString event = payload.value("event").toString();
    if (!event.isEmpty()) {
        setGroupStatus(QString("群事件：%1").arg(event), false);
    }
    refreshGroupList();
}

void AppController::onGroupChatMsg(std::shared_ptr<GroupChatMsg> msg)
{
    if (!msg || !msg->_msg) {
        return;
    }

    auto textMsg = std::make_shared<TextChatData>(msg->_msg->_msg_id, msg->_msg->_msg_content,
                                                  msg->_msg->_from_uid, static_cast<int>(msg->_group_id),
                                                  msg->_from_name);
    _gateway.userMgr()->AppendGroupChatMsg(msg->_group_id, textMsg);

    int pseudoUid = 0;
    for (auto it = _group_uid_map.cbegin(); it != _group_uid_map.cend(); ++it) {
        if (it.value() == msg->_group_id) {
            pseudoUid = it.key();
            break;
        }
    }
    if (pseudoUid != 0) {
        const QString preview = MessageContentCodec::toPreviewText(msg->_msg->_msg_content);
        _group_list_model.updateLastMessage(pseudoUid, preview);
    }

    if (msg->_group_id != _current_group_id) {
        return;
    }
    _message_model.appendMessage(textMsg, _gateway.userMgr()->GetUid());
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
        case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP: return "更新群公告";
        case ID_MUTE_GROUP_MEMBER_RSP: return "禁言成员";
        case ID_SET_GROUP_ADMIN_RSP: return "设置管理员";
        case ID_KICK_GROUP_MEMBER_RSP: return "踢出成员";
        case ID_QUIT_GROUP_RSP: return "退出群聊";
        default: return "群操作";
        }
    };

    if (error != ErrorCodes::SUCCESS) {
        setGroupStatus(QString("%1失败（错误码:%2）").arg(action_text(reqId)).arg(error), true);
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
        if (_current_group_id > 0) {
            auto groupInfo = _gateway.userMgr()->GetGroupById(_current_group_id);
            if (groupInfo) {
                _message_model.setMessages(groupInfo->_chat_msgs, _gateway.userMgr()->GetUid());
            }
        }
        setGroupStatus("历史消息已加载", false);
        break;
    }
    case ID_UPDATE_GROUP_ANNOUNCEMENT_RSP:
        setGroupStatus("群公告已更新", false);
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
        refreshGroupList();
        setCurrentGroup(0, "");
        if (_current_chat_uid > 0) {
            loadCurrentChatMessages();
        } else {
            _message_model.clear();
            setCurrentChatPeerName("");
            setCurrentChatPeerIcon("qrc:/res/head_1.jpg");
        }
        break;
    case ID_GROUP_CHAT_MSG_RSP:
    case ID_GET_GROUP_LIST_RSP:
    default:
        break;
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

    const auto groups = _gateway.userMgr()->GetGroupListPerPage();
    std::vector<std::shared_ptr<FriendInfo>> converted;
    converted.reserve(groups.size());
    for (const auto &group : groups) {
        if (!group || group->_group_id <= 0) {
            continue;
        }
        const int pseudoUid = -static_cast<int>(group->_group_id);
        auto info = std::make_shared<FriendInfo>(pseudoUid,
                                                 group->_name,
                                                 group->_name,
                                                 "qrc:/res/chat_icon.png",
                                                 0,
                                                 group->_announcement,
                                                 group->_announcement,
                                                 group->_last_msg);
        converted.push_back(info);
        _group_uid_map.insert(pseudoUid, group->_group_id);
    }

    _group_list_model.setFriends(converted);
    _gateway.userMgr()->UpdateGroupLoadedCount();
}

void AppController::loadCurrentChatMessages()
{
    if (_current_chat_uid <= 0) {
        _message_model.clear();
        return;
    }

    auto friendInfo = _gateway.userMgr()->GetFriendById(_current_chat_uid);
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!friendInfo || !selfInfo) {
        _message_model.clear();
        return;
    }

    setCurrentChatPeerName(friendInfo->_name);
    setCurrentChatPeerIcon(friendInfo->_icon);
    _message_model.setMessages(friendInfo->_chat_msgs, selfInfo->_uid);
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
                                      const QString &back, int sex)
{
    const QString normalizedIcon = normalizeIconPath(icon);
    if (_current_contact_uid == uid
        && _current_contact_name == name
        && _current_contact_nick == nick
        && _current_contact_icon == normalizedIcon
        && _current_contact_back == back
        && _current_contact_sex == sex) {
        return;
    }

    _current_contact_uid = uid;
    _current_contact_name = name;
    _current_contact_nick = nick;
    _current_contact_icon = normalizedIcon;
    _current_contact_back = back;
    _current_contact_sex = sex;
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
    }
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

void AppController::setCurrentGroup(qint64 groupId, const QString &name)
{
    const QString normalizedName = (groupId > 0) ? name : QString();
    if (_current_group_id == groupId && _current_group_name == normalizedName) {
        return;
    }

    _current_group_id = groupId;
    _current_group_name = normalizedName;
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

    QString errorText;
    if (!_chat_controller.dispatchChatText(packet, &errorText)) {
        if (!errorText.isEmpty()) {
            setTip(errorText, true);
        }
        return false;
    }

    auto msg = std::make_shared<TextChatData>(packet.msgId, content, packet.fromUid, packet.toUid);
    _gateway.userMgr()->AppendFriendChatMsg(_current_chat_uid, {msg});
    _message_model.appendMessage(msg, packet.fromUid);

    const QString resolvedPreview = previewText.isEmpty() ? MessageContentCodec::toPreviewText(content) : previewText;
    _chat_list_model.updateLastMessage(_current_chat_uid, resolvedPreview);
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
    const DecodedMessageContent decoded = MessageContentCodec::decode(content);

    QJsonObject msgObj;
    msgObj["msgid"] = msgId;
    msgObj["content"] = content;
    msgObj["msgtype"] = decoded.type.isEmpty() ? "text" : decoded.type;
    msgObj["mentions"] = QJsonArray();
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
    auto msg = std::make_shared<TextChatData>(msgId, content, selfInfo->_uid, 0, senderName);
    _gateway.userMgr()->AppendGroupChatMsg(_current_group_id, msg);
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
        _group_list_model.updateLastMessage(pseudoUid, resolvedPreview);
    }

    return true;
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


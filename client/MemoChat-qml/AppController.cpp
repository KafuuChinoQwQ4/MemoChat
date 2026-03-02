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
      _current_chat_uid(0),
      _chat_list_model(this),
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
    connect(&_apply_request_model, &ApplyRequestModel::unapprovedCountChanged,
            this, &AppController::pendingApplyChanged);

    connect(&_register_countdown_timer, &QTimer::timeout,
            this, &AppController::onRegisterCountdownTimeout);
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

bool AppController::hasCurrentChat() const
{
    return _current_chat_uid > 0;
}

FriendListModel *AppController::chatListModel()
{
    return &_chat_list_model;
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

void AppController::switchToLogin()
{
    _register_countdown_timer.stop();
    _gateway.userMgr()->ResetSession();
    if (_register_success_page) {
        _register_success_page = false;
        emit registerSuccessPageChanged();
    }
    setBusy(false);
    setPage(LoginPage);
    setTip("", false);
    _chat_list_model.clear();
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
    setContactPane(ApplyRequestPane);
    _can_load_more_chats = false;
    emit canLoadMoreChatsChanged();
    _can_load_more_contacts = false;
    emit canLoadMoreContactsChanged();
    setCurrentContact(0, "", "", "qrc:/res/head_1.jpg", "", 0);
    _current_chat_uid = 0;
    setCurrentChatPeerName("");
    _current_user_desc.clear();
}

void AppController::switchToRegister()
{
    _register_countdown_timer.stop();
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
        setCurrentChatPeerName("");
        _message_model.clear();
        return;
    }

    _current_chat_uid = item.value("uid").toInt();
    setCurrentChatPeerName(item.value("name").toString());
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
    if (_current_chat_uid <= 0) {
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

    if (!dispatchChatContent(content, content)) {
        setTip("消息发送失败", true);
    }
}

void AppController::sendImageMessage()
{
    if (_current_chat_uid <= 0) {
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
    if (!dispatchChatContent(encoded, "[图片]")) {
        setTip("图片发送失败", true);
    }
}

void AppController::sendFileMessage()
{
    if (_current_chat_uid <= 0) {
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
    if (_chat_list_model.count() > 0) {
        selectChatIndex(0);
    } else {
        _current_chat_uid = 0;
        setCurrentChatPeerName("");
    }
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

    _gateway.tcpMgr()->CloseConnection();
    switchToLogin();
    setTip("同账号异地登录，该终端下线", true);
}

void AppController::onConnectionClosed()
{
    if (_page != ChatPage) {
        return;
    }

    switchToLogin();
    setTip("心跳超时或连接断开，请重新登录", true);
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
}

void AppController::refreshApplyModel()
{
    const auto applyList = _gateway.userMgr()->GetApplyListSnapshot();
    _apply_request_model.setApplies(applyList);
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


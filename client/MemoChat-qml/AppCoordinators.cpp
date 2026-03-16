#include "AppCoordinators.h"

#include "AppController.h"
#include "LocalFilePickerService.h"
#include "MediaUploadService.h"
#include "MessageContentCodec.h"
#include "IChatTransport.h"
#include "httpmgr.h"
#include "usermgr.h"

#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QVariantMap>

AppSessionCoordinator::AppSessionCoordinator(AppController& controller)
    : _app(controller) {
}

void AppSessionCoordinator::login(const QString& email, const QString& password)
{
    if (!_app.checkEmailValid(email) || !_app.checkPwdValid(password)) {
        return;
    }

    _app._pending_uid = 0;
    _app._pending_token.clear();
    _app._pending_login_ticket.clear();
    _app._pending_trace_id.clear();
    _app._chat_endpoints.clear();
    _app._chat_endpoint_index = -1;
    _app._chat_server_name.clear();
    _app._login_started_ms = QDateTime::currentMSecsSinceEpoch();
    _app._http_login_finished_ms = 0;
    _app._chat_connect_started_ms = 0;
    _app._chat_connect_finished_ms = 0;
    _app._chat_login_timeout_timer.stop();
    _app._ignore_next_login_disconnect = true;
    _app._gateway.chatTransport()->CloseConnection();
    _app.setBusy(true);
    _app.setTip("", false);
    _app._auth_controller.sendLogin(email, password);
}

void AppSessionCoordinator::requestRegisterCode(const QString& email)
{
    if (!_app.checkEmailValid(email)) {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendVerifyCode(email, Modules::REGISTERMOD);
}

void AppSessionCoordinator::registerUser(const QString& user, const QString& email, const QString& password,
                                         const QString& confirm, const QString& verifyCode)
{
    if (!_app.checkUserValid(user) || !_app.checkEmailValid(email) || !_app.checkPwdValid(password)) {
        return;
    }
    if (password != confirm) {
        _app.setTip("密码和确认密码不匹配", true);
        return;
    }
    if (!_app.checkVerifyCodeValid(verifyCode)) {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendRegister(user, email, password, confirm, verifyCode);
}

void AppSessionCoordinator::requestResetCode(const QString& email)
{
    if (!_app.checkEmailValid(email)) {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendVerifyCode(email, Modules::RESETMOD);
}

void AppSessionCoordinator::resetPassword(const QString& user, const QString& email, const QString& password,
                                          const QString& verifyCode)
{
    if (!_app.checkUserValid(user) || !_app.checkEmailValid(email) || !_app.checkPwdValid(password)
        || !_app.checkVerifyCodeValid(verifyCode)) {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendResetPassword(user, email, password, verifyCode);
}

ContactCoordinatorShell::ContactCoordinatorShell(AppController& controller)
    : _app(controller) {
}

void ContactCoordinatorShell::searchUser(const QString& uidText)
{
    QString errorText;
    if (!_app._contact_controller.sendSearchUser(uidText, &errorText)) {
        _app.clearSearchResultOnly();
        _app.setSearchStatus(errorText, true);
        return;
    }
    _app.clearSearchResultOnly();
    _app.setSearchPending(true);
    _app.setSearchStatus("搜索中...", false);
}

void ContactCoordinatorShell::requestAddFriend(int uid, const QString& bakName, const QVariantList& labels)
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        return;
    }
    if (uid == selfInfo->_uid) {
        _app.setSearchStatus("不能添加自己", true);
        return;
    }
    if (_app._gateway.userMgr()->CheckFriendById(uid)) {
        _app.setSearchStatus("已是好友，无需重复申请", false);
        return;
    }

    _app._contact_controller.sendAddFriend(selfInfo->_uid, selfInfo->_name, uid, bakName, labels);
    _app.setSearchStatus("好友申请已发送", false);
}

void ContactCoordinatorShell::approveFriend(int uid, const QString& backName, const QVariantList& labels)
{
    if (uid <= 0) {
        _app.setAuthStatus("非法好友申请", true);
        return;
    }
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        _app.setAuthStatus("用户状态异常，请重新登录", true);
        return;
    }
    if (_app._gateway.userMgr()->CheckFriendById(uid)) {
        _app._apply_request_model.markApproved(uid);
        _app.setAuthStatus("已是好友", false);
        return;
    }

    QString remark = backName.trimmed();
    if (remark.isEmpty()) {
        remark = _app._apply_request_model.nameByUid(uid);
    }
    if (remark.isEmpty()) {
        remark = "MemoChat好友";
    }

    _app._contact_controller.sendApproveFriend(selfInfo->_uid, uid, remark, labels);
    _app._apply_request_model.setPending(uid, true);
    _app.setAuthStatus("好友认证请求已发送", false);
}

CallCoordinator::CallCoordinator(AppController& controller)
    : _app(controller) {
}

bool CallCoordinator::ensureCallTargetFromCurrentChat()
{
    if (_app.hasCurrentContact()) {
        return true;
    }
    if (_app._current_group_id > 0 || _app._current_chat_uid <= 0) {
        return false;
    }
    auto friendInfo = _app._gateway.userMgr()->GetFriendById(_app._current_chat_uid);
    if (!friendInfo) {
        return false;
    }
    _app.setCurrentContact(friendInfo->_uid, friendInfo->_name, friendInfo->_nick, friendInfo->_icon,
                           friendInfo->_back, friendInfo->_sex, friendInfo->_user_id);
    return _app.hasCurrentContact();
}

void CallCoordinator::startCallFlow(const QString& callType)
{
    _app.startCallFlow(callType);
}

void CallCoordinator::startVoiceChat()
{
    if (ensureCallTargetFromCurrentChat()) {
        startCallFlow("voice");
    }
}

void CallCoordinator::startVideoChat()
{
    if (ensureCallTargetFromCurrentChat()) {
        startCallFlow("video");
    }
}

void CallCoordinator::acceptIncomingCall()
{
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._call_session_model.callId().isEmpty()) {
        return;
    }
    _app._call_controller.acceptCall(selfInfo->_uid, _app._gateway.userMgr()->GetToken(), _app._call_session_model.callId());
    _app.setAuthStatus("正在接听通话", false);
}

void CallCoordinator::rejectIncomingCall()
{
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._call_session_model.callId().isEmpty()) {
        return;
    }
    _app._call_controller.rejectCall(selfInfo->_uid, _app._gateway.userMgr()->GetToken(), _app._call_session_model.callId());
}

void CallCoordinator::endCurrentCall()
{
    const auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._call_session_model.callId().isEmpty()) {
        return;
    }
    _app._livekit_bridge.leaveRoom();
    if (!_app._call_session_model.active() && !_app._call_session_model.incoming()) {
        _app._call_controller.cancelCall(selfInfo->_uid, _app._gateway.userMgr()->GetToken(), _app._call_session_model.callId());
        return;
    }
    _app._call_controller.hangupCall(selfInfo->_uid, _app._gateway.userMgr()->GetToken(), _app._call_session_model.callId());
}

void CallCoordinator::toggleCallMuted()
{
    _app._livekit_bridge.toggleMic();
    _app._call_session_model.setMuted(!_app._call_session_model.muted());
}

void CallCoordinator::toggleCallCamera()
{
    if (_app._call_session_model.callType() != QStringLiteral("video")) {
        return;
    }
    _app._livekit_bridge.toggleCamera();
    _app._call_session_model.setCameraEnabled(!_app._call_session_model.cameraEnabled());
}

ProfileCoordinator::ProfileCoordinator(AppController& controller)
    : _app(controller) {
}

void ProfileCoordinator::chooseAvatar()
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        _app.setSettingsStatus("用户状态异常，请重新登录", true);
        return;
    }

    QString avatarUrl;
    QString errorText;
    if (!LocalFilePickerService::pickAvatarUrl(&avatarUrl, &errorText)) {
        if (!errorText.isEmpty()) {
            _app.setSettingsStatus(errorText, true);
        }
        return;
    }

    _app._gateway.userMgr()->UpdateIcon(avatarUrl);
    const QString normalized = _app.normalizeIconPath(avatarUrl);
    if (_app._current_user_icon != normalized) {
        _app._current_user_icon = normalized;
        emit _app.currentUserChanged();
    }
    _app.setSettingsStatus("已选择新头像，点击保存后同步", false);
}

void ProfileCoordinator::saveProfile(const QString& nick, const QString& desc)
{
    _app.saveProfile(nick, desc);
}

GroupCoordinator::GroupCoordinator(AppController& controller)
    : _app(controller) {
}

void GroupCoordinator::createGroup(const QString& name, const QVariantList& memberUserIdList)
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo) {
        _app.setGroupStatus("用户状态异常，请重新登录", true);
        return;
    }
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty() || trimmedName.size() > 64) {
        _app.setGroupStatus("群名称长度需在1-64之间", true);
        return;
    }

    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    QJsonArray memberUserIds;
    for (const auto& one : memberUserIdList) {
        const QString userId = one.toString().trimmed();
        if (userId.isEmpty()) {
            continue;
        }
        if (!kUserIdPattern.match(userId).hasMatch()) {
            _app.setGroupStatus(QString("成员ID格式非法：%1").arg(userId), true);
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
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_CREATE_GROUP_REQ, QJsonDocument(obj).toJson(QJsonDocument::Compact));
    _app.setGroupStatus("正在创建群聊...", false);
}

void GroupCoordinator::inviteGroupMember(const QString& userId, const QString& reason)
{
    Q_UNUSED(reason);
    _app.inviteGroupMember(userId, reason);
}

void GroupCoordinator::applyJoinGroup(const QString& groupCode, const QString& reason)
{
    Q_UNUSED(reason);
    _app.applyJoinGroup(groupCode, reason);
}

void GroupCoordinator::reviewGroupApply(qint64 applyId, bool agree)
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || applyId <= 0) {
        _app.setGroupStatus("审核参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["apply_id"] = applyId;
    obj["agree"] = agree;
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_REVIEW_GROUP_APPLY_REQ, QJsonDocument(obj).toJson(QJsonDocument::Compact));
    _app.setGroupStatus("审核请求已发送", false);
}

void GroupCoordinator::sendGroupTextMessage(const QString& text)
{
    if (_app._current_group_id <= 0) {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    _app.sendTextMessage(text);
}

void GroupCoordinator::sendGroupImageMessage()
{
    if (_app._current_group_id <= 0) {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    _app.sendImageMessage();
}

void GroupCoordinator::sendGroupFileMessage()
{
    if (_app._current_group_id <= 0) {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    _app.sendFileMessage();
}

void GroupCoordinator::updateGroupAnnouncement(const QString& announcement)
{
    if (_app._current_group_id <= 0) {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    _app.updateGroupAnnouncement(announcement);
}

void GroupCoordinator::setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits)
{
    _app.setGroupAdmin(userId, isAdmin, permissionBits);
}

void GroupCoordinator::muteGroupMember(const QString& userId, int muteSeconds)
{
    _app.muteGroupMember(userId, muteSeconds);
}

void GroupCoordinator::kickGroupMember(const QString& userId)
{
    _app.kickGroupMember(userId);
}

void GroupCoordinator::quitCurrentGroup()
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _app._current_group_id <= 0) {
        _app.setGroupStatus("请选择群聊", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_app._current_group_id);
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_QUIT_GROUP_REQ, QJsonDocument(obj).toJson(QJsonDocument::Compact));
}

MediaCoordinator::MediaCoordinator(AppController& controller)
    : _app(controller) {
}

void MediaCoordinator::sendTextMessage(const QString& text)
{
    if (_app._current_chat_uid <= 0 && _app._current_group_id <= 0) {
        return;
    }

    QString content = text;
    if (content.trimmed().isEmpty()) {
        return;
    }
    if (content.size() > 1024) {
        _app.setTip("单条消息不能超过1024字符", true);
        return;
    }

    if (_app._current_group_id > 0) {
        if (!_app._reply_to_msg_id.isEmpty()) {
            content = MessageContentCodec::encodeReplyText(content, _app._reply_to_msg_id,
                                                           _app._reply_target_name, _app._reply_preview_text);
        }
        if (!_app.dispatchGroupChatContent(content, QString())) {
            _app.setTip("群消息发送失败", true);
        } else {
            _app.cancelReplyMessage();
        }
        return;
    }

    if (!_app.dispatchChatContent(content, content)) {
        _app.setTip("消息发送失败", true);
    }
}

void MediaCoordinator::sendCurrentComposerPayload(const QString& text)
{
    if (_app._current_chat_uid <= 0 && _app._current_group_id <= 0) {
        return;
    }
    if (_app._media_upload_in_progress) {
        _app.setTip("已有文件上传中，请稍后", true);
        return;
    }
    if (!_app._current_pending_attachments.isEmpty()) {
        _app._pending_send_dialog_uid = _app.currentDialogUid();
        _app._pending_send_chat_uid = _app._current_chat_uid;
        _app._pending_send_group_id = _app._current_group_id;
        _app._pending_send_queue = _app._current_pending_attachments;
        _app._pending_send_total_count = _app._pending_send_queue.size();
        _app.processPendingAttachmentQueue();
        return;
    }
    sendTextMessage(text);
}

void MediaCoordinator::sendImageMessage()
{
    if (_app._current_chat_uid <= 0 && _app._current_group_id <= 0) {
        return;
    }
    if (_app._media_upload_in_progress) {
        _app.setTip("已有文件上传中，请稍后", true);
        return;
    }
    QVariantList attachments;
    QString errorText;
    if (!LocalFilePickerService::pickImageUrls(&attachments, &errorText)) {
        if (!errorText.isEmpty()) {
            _app.setTip(errorText, true);
        }
        return;
    }
    _app.addPendingAttachments(attachments);
}

void MediaCoordinator::sendFileMessage()
{
    if (_app._current_chat_uid <= 0 && _app._current_group_id <= 0) {
        return;
    }
    if (_app._media_upload_in_progress) {
        _app.setTip("已有文件上传中，请稍后", true);
        return;
    }
    QVariantList attachments;
    QString errorText;
    if (!LocalFilePickerService::pickFileUrls(&attachments, &errorText)) {
        if (!errorText.isEmpty()) {
            _app.setTip(errorText, true);
        }
        return;
    }
    _app.addPendingAttachments(attachments);
}

void MediaCoordinator::removePendingAttachment(const QString& attachmentId)
{
    if (_app._media_upload_in_progress) {
        return;
    }
    _app.removePendingAttachmentById(attachmentId);
}

void MediaCoordinator::clearPendingAttachments()
{
    if (_app._media_upload_in_progress) {
        return;
    }
    const int dialogUid = _app.currentDialogUid();
    if (dialogUid == 0) {
        return;
    }
    _app._dialog_pending_attachment_map.remove(dialogUid);
    _app.setCurrentPendingAttachments(QVariantList());
}

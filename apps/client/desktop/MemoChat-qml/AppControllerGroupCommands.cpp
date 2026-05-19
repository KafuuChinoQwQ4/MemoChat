#include "AppController.h"
#include "AppCoordinators.h"
#include "IChatTransport.h"
#include "LocalFilePickerService.h"
#include "MediaUploadService.h"
#include "usermgr.h"

#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUuid>
#include <QtConcurrent>
#include <QtGlobal>

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
}

void AppController::refreshGroupList()
{
    if (!_groups_ready) {
        bootstrapGroups();
        return;
    }
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || selfInfo->_uid <= 0) {
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
    if (_group_history_loading) {
        qInfo() << "Skip group history request because one is already loading, group id:" << _current_group_id;
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
    _group_history_loading = true;
    qInfo() << "Requesting group history, group id:" << _current_group_id
            << "before seq:" << _group_history_before_seq
            << "private history loading:" << _private_history_loading;
    setPrivateHistoryLoading(true);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_GROUP_HISTORY_REQ, payload);
}

void AppController::requestGroupHistoryForBootstrap(qint64 groupId)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || groupId <= 0) {
        return;
    }
    if (groupId == _current_group_id && _group_history_loading) {
        qInfo() << "Skip bootstrap group history because current group is already loading, group id:" << groupId;
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(groupId);
    obj["before_ts"] = 0;
    obj["before_seq"] = static_cast<qint64>(0);
    obj["limit"] = 50;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    if (groupId == _current_group_id) {
        _group_history_loading = true;
        setPrivateHistoryLoading(true);
    }
    qInfo() << "Requesting bootstrap group history, group id:" << groupId
            << "current group id:" << _current_group_id
            << "private history loading:" << _private_history_loading;
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

void AppController::updateGroupIcon(int source)
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
    bool ok = false;
    if (source == 0) {
        ok = LocalFilePickerService::pickAvatarUrl(&avatarUrl, &errorText);
    } else if (source == 1) {
        ok = LocalFilePickerService::pickAvatarFromScreen(&avatarUrl, &errorText);
    } else {
        ok = LocalFilePickerService::pickAvatarFromWebcam(&avatarUrl, &errorText);
    }
    if (!ok) {
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

void AppController::dissolveCurrentGroup()
{
    _group_coordinator->dissolveCurrentGroup();
}

void AppController::clearGroupStatus()
{
    setGroupStatus(QString(), false);
}

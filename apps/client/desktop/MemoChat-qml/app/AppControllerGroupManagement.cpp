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
#include <QtConcurrent>
#include <QtGlobal>

namespace
{
struct MediaUploadTaskResult
{
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
} // namespace

void AppController::updateGroupAnnouncement(const QString& announcement)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _group_state.currentId <= 0)
    {
        setGroupStatus("请选择群聊", true);
        return;
    }
    if (!currentGroupCanChangeInfo())
    {
        setGroupStatus("没有修改群资料权限", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_group_state.currentId);
    obj["announcement"] = announcement.left(1000);
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_UPDATE_GROUP_ANNOUNCEMENT_REQ, payload);
}

void AppController::updateGroupIcon(int source)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || _group_state.currentId <= 0)
    {
        setGroupStatus("请选择群聊", true);
        return;
    }
    auto groupInfo = _gateway.userMgr()->GetGroupById(_group_state.currentId);
    if (!groupInfo)
    {
        setGroupStatus("群聊不存在或已失效", true);
        return;
    }
    if (!currentGroupCanChangeInfo())
    {
        setGroupStatus("没有修改群资料权限", true);
        return;
    }
    if (_media_upload_state.groupIconUploadInProgress)
    {
        setGroupStatus("群头像上传中，请稍候", false);
        return;
    }

    QString avatarUrl;
    QString errorText;
    bool ok = false;
    if (source == 0)
    {
        ok = LocalFilePickerService::pickAvatarUrl(&avatarUrl, &errorText);
    }
    else if (source == 1)
    {
        ok = LocalFilePickerService::pickAvatarFromScreen(&avatarUrl, &errorText);
    }
    else
    {
        ok = LocalFilePickerService::pickAvatarFromWebcam(&avatarUrl, &errorText);
    }
    if (!ok)
    {
        if (!errorText.isEmpty())
        {
            setGroupStatus(errorText, true);
        }
        return;
    }

    if (_pending_login_state.token.trimmed().isEmpty())
    {
        setGroupStatus("登录态失效，请重新登录", true);
        return;
    }

    const int selfUid = selfInfo->_uid;
    const qint64 targetGroupId = _group_state.currentId;
    const QString uploadToken = _pending_login_state.token;
    _media_upload_state.groupIconUploadInProgress = true;
    setGroupStatus("群头像上传中...", false);

    auto* watcher = new QFutureWatcher<MediaUploadTaskResult>(this);
    connect(watcher,
            &QFutureWatcher<MediaUploadTaskResult>::finished,
            this,
            [this, watcher, selfUid, targetGroupId]()
            {
                const MediaUploadTaskResult result = watcher->future().result();
                watcher->deleteLater();
                _media_upload_state.groupIconUploadInProgress = false;
                if (!result.ok || result.uploaded.remoteUrl.isEmpty())
                {
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

    const auto future = QtConcurrent::run(
        [avatarUrl, selfUid, uploadToken]()
        {
            MediaUploadTaskResult result;
            QString uploadErr;
            result.ok = MediaUploadService::uploadLocalFile(
                avatarUrl,
                QStringLiteral("avatar"), selfUid, uploadToken, &result.uploaded, &uploadErr);
            if (!result.ok)
            {
                result.errorText = uploadErr;
            }
            return result;
        });
    watcher->setFuture(future);
}

void AppController::setGroupAdmin(const QString& userId, bool isAdmin, qint64 permissionBits)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedUserId = userId.trimmed();
    if (!selfInfo || _group_state.currentId <= 0 || !kUserIdPattern.match(trimmedUserId).hasMatch())
    {
        setGroupStatus("设置管理员参数非法", true);
        return;
    }
    if (!currentGroupCanManageAdmins())
    {
        setGroupStatus("没有管理管理员权限", true);
        return;
    }
    qint64 normalizedPermBits = permissionBits;
    if (isAdmin)
    {
        if (normalizedPermBits <= 0)
        {
            normalizedPermBits = kDefaultAdminPermBits;
        }
        normalizedPermBits &= kOwnerPermBits;
        if (normalizedPermBits <= 0)
        {
            normalizedPermBits = kDefaultAdminPermBits;
        }
    }
    else
    {
        normalizedPermBits = 0;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_group_state.currentId);
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

void AppController::muteGroupMember(const QString& userId, int muteSeconds)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedUserId = userId.trimmed();
    if (!selfInfo || _group_state.currentId <= 0 || !kUserIdPattern.match(trimmedUserId).hasMatch())
    {
        setGroupStatus("禁言参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_group_state.currentId);
    obj["target_user_id"] = trimmedUserId;
    obj["mute_seconds"] = qMax(0, muteSeconds);
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_MUTE_GROUP_MEMBER_REQ, payload);
}

void AppController::kickGroupMember(const QString& userId)
{
    static const QRegularExpression kUserIdPattern("^u[1-9][0-9]{8}$");
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    const QString trimmedUserId = userId.trimmed();
    if (!selfInfo || _group_state.currentId <= 0 || !kUserIdPattern.match(trimmedUserId).hasMatch())
    {
        setGroupStatus("踢人参数非法", true);
        return;
    }
    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["groupid"] = static_cast<qint64>(_group_state.currentId);
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

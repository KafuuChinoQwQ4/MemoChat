#include "AppController.h"
#include "AppCoordinators.h"
#include "MediaUploadService.h"
#include "usermgr.h"

#include <QFileInfo>
#include <QFutureWatcher>
#include <QUrl>
#include <QtConcurrent>

namespace {
struct ProfileAvatarUploadResult {
    bool ok = false;
    UploadedMediaInfo uploaded;
    QString errorText;
};
}

void AppController::chooseAvatar(int source)
{
    if (source == 0) {
        _profile_coordinator->chooseAvatar();
    } else {
        setSettingsStatus("屏幕截图/拍照功能开发中", false);
    }
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

        auto *watcher = new QFutureWatcher<ProfileAvatarUploadResult>(this);
        connect(watcher, &QFutureWatcher<ProfileAvatarUploadResult>::finished, this,
                [this, watcher, selfUid, selfName, nick, desc]() {
            const ProfileAvatarUploadResult result = watcher->future().result();
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
            ProfileAvatarUploadResult result;
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
    setSettingsStatus(QString(), false);
}

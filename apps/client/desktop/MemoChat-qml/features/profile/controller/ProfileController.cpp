#include "ProfileController.h"
#include "ClientGateway.h"
#include "IconPathUtils.h"
#include "LocalFilePickerService.h"
#include "MediaUploadService.h"
#include "ProfileRequestPayloads.h"
#include "httpmgr.h"
#include "usermgr.h"
#include "global.h"
#include <QFileInfo>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrl>
#include <QtConcurrent>
#include <memory>
#include <utility>

namespace
{
ProfileCurrentUserState currentUserSnapshot(const ProfileStatePort& port)
{
    if (port.snapshot)
    {
        return port.snapshot();
    }
    return {};
}

bool parseProfileResponse(const QString& res, QJsonObject* obj)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        return false;
    }
    if (obj)
    {
        *obj = doc.object();
    }
    return true;
}
} // namespace

ProfileController::ProfileController(ClientGateway* gateway, QObject* parent)
    : QObject(parent)
    , _gateway(gateway)
{
}

QString ProfileController::statusText() const
{
    return _status_text;
}

bool ProfileController::statusError() const
{
    return _status_error;
}

void ProfileController::chooseAvatar(int source)
{
    const ProfileCommandSnapshot snapshot =
        _command_port.snapshot ? _command_port.snapshot() : ProfileCommandSnapshot{};
    if (snapshot.selfUid <= 0)
    {
        setStatus("用户状态异常，请重新登录", true);
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
            setStatus(errorText, true);
        }
        return;
    }

    if (_command_port.updateLocalIcon)
    {
        _command_port.updateLocalIcon(avatarUrl);
    }
    setStatus("已选择新头像，点击保存后同步", false);
}

void ProfileController::saveProfile(const QString& nick, const QString& desc)
{
    const ProfileCommandSnapshot snapshot =
        _command_port.snapshot ? _command_port.snapshot() : ProfileCommandSnapshot{};
    if (snapshot.selfUid <= 0)
    {
        setStatus("用户状态异常，请重新登录", true);
        return;
    }

    QString errorText;
    if (!validateProfile(nick, desc, &errorText))
    {
        setStatus(errorText, true);
        return;
    }

    const QString iconForSave = snapshot.currentIcon;
    const QUrl iconUrl(iconForSave);
    if (iconUrl.isLocalFile() || QFileInfo(iconForSave).isAbsolute())
    {
        if (snapshot.avatarUploadInProgress)
        {
            setStatus("头像上传中，请稍候", false);
            return;
        }
        if (snapshot.uploadToken.trimmed().isEmpty())
        {
            setStatus("登录态失效，请重新登录", true);
            return;
        }
        if (!_command_port.uploadAvatar)
        {
            setStatus("头像上传服务未就绪", true);
            return;
        }

        if (_command_port.setAvatarUploadInProgress)
        {
            _command_port.setAvatarUploadInProgress(true);
        }
        setStatus("头像上传中...", false);

        auto* watcher = new QFutureWatcher<UploadedMediaInfo>(this);
        auto* uploadError = new QString();
        connect(watcher,
                &QFutureWatcher<UploadedMediaInfo>::finished,
                this,
                [this, watcher, uploadError, snapshot, nick, desc]()
                {
                    const UploadedMediaInfo uploaded = watcher->future().result();
                    watcher->deleteLater();
                    const QString errorText = *uploadError;
                    delete uploadError;
                    if (_command_port.setAvatarUploadInProgress)
                    {
                        _command_port.setAvatarUploadInProgress(false);
                    }
                    if (uploaded.remoteUrl.isEmpty())
                    {
                        setStatus(errorText.isEmpty() ? "头像上传失败" : errorText, true);
                        return;
                    }
                    sendSaveProfile(snapshot.selfUid,
                                    snapshot.uploadToken,
                                    snapshot.selfName,
                                    nick,
                                    desc,
                                    uploaded.remoteUrl);
                    setStatus("资料同步中...", false);
                });

        const QString uploadPath = iconForSave;
        const int selfUid = snapshot.selfUid;
        const QString uploadToken = snapshot.uploadToken;
        watcher->setFuture(QtConcurrent::run(
            [this, uploadPath, selfUid, uploadToken, uploadError]()
            {
                UploadedMediaInfo uploaded;
                if (!_command_port.uploadAvatar(uploadPath, selfUid, uploadToken, &uploaded, uploadError))
                {
                    return UploadedMediaInfo{};
                }
                return uploaded;
            }));
        return;
    }

    if (snapshot.uploadToken.trimmed().isEmpty())
    {
        setStatus("登录状态已过期，请重新登录", true);
        return;
    }
    sendSaveProfile(snapshot.selfUid, snapshot.uploadToken, snapshot.selfName, nick, desc, iconForSave);
    setStatus("资料同步中...", false);
}

void ProfileController::clearStatus()
{
    clearStatusState();
}

void ProfileController::clearStatusState()
{
    setStatus(QString(), false);
}

bool ProfileController::validateProfile(const QString& nick, const QString& desc, QString* errorText) const
{
    return memochat::profile_payload::validateProfile(nick, desc, errorText);
}

void ProfileController::sendSaveProfile(int uid,
                                        const QString& token,
                                        const QString& name,
                                        const QString& nick,
                                        const QString& desc,
                                        const QString& icon) const
{
    if (!_gateway || !_gateway->httpMgr())
    {
        return;
    }

    const QJsonObject payload = memochat::profile_payload::buildSaveProfilePayload(name, nick, desc, icon);

    _gateway->httpMgr()->PostHttpReq(QUrl(gate_url_prefix + "/user_update_profile"),
                                     payload,
                                     ReqId::ID_UPDATE_PROFILE,
                                     Modules::SETTINGSMOD,
                                     QStringLiteral("profile"));
}

void ProfileController::setStatus(const QString& text, bool isError)
{
    if (_status_text == text && _status_error == isError)
    {
        return;
    }

    _status_text = text;
    _status_error = isError;
    emit statusChanged();
}

void ProfileController::syncStatus(const QString& text, bool isError)
{
    setStatus(text, isError);
}

void ProfileController::setCommandPort(ProfileCommandPort port)
{
    _command_port = std::move(port);
}

void ProfileController::setStatePort(ProfileStatePort port)
{
    _state_port = std::move(port);
}

void ProfileController::handleSettingsHttpFinished(ReqId id, const QString& res, ErrorCodes err)
{
    if (id != ReqId::ID_UPDATE_PROFILE)
    {
        return;
    }

    if (err != ErrorCodes::SUCCESS)
    {
        setStatus("资料同步失败：网络错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseProfileResponse(res, &obj))
    {
        setStatus("资料同步失败：响应解析错误", true);
        return;
    }

    const int error = obj.value(QStringLiteral("error")).toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        setStatus("资料同步失败", true);
        return;
    }

    const ProfileCurrentUserState snapshot = currentUserSnapshot(_state_port);
    applyCurrentUserProfile(snapshot.uid > 0 ? snapshot.uid : snapshot.pendingUid,
                            snapshot.name,
                            obj.value(QStringLiteral("nick")).toString(snapshot.nick),
                                      obj.value(QStringLiteral("icon")).toString(snapshot.icon),
                                                obj.value(QStringLiteral("desc")).toString(snapshot.desc),
                                                          snapshot.userId,
                                                          snapshot.sex,
                                                          false);
    setStatus("资料已同步", false);
}

void ProfileController::applyCurrentUserProfile(const QJsonObject& profile, bool preserveExistingIcon)
{
    if (profile.isEmpty())
    {
        return;
    }

    const ProfileCurrentUserState snapshot = currentUserSnapshot(_state_port);
    applyCurrentUserProfile(profile.value(
        QStringLiteral("uid"))
            .toInt(snapshot.pendingUid),
        profile.value(QStringLiteral("name")).toString(snapshot.name),
                      profile.value(QStringLiteral("nick")).toString(snapshot.nick),
                                    profile.value(QStringLiteral("icon")).toString(),
                                                  profile.value(QStringLiteral("desc")).toString(snapshot.desc),
                                                                profile.value(
                                                                    QStringLiteral("user_id"))
                                                                        .toString(snapshot.userId),
                                                                    profile.value(QStringLiteral("sex")).toInt(0),
                                                                                  preserveExistingIcon);
}

void ProfileController::applyCurrentUserProfile(int uid,
                                                const QString& name,
                                                const QString& nick,
                                                const QString& icon,
                                                const QString& desc,
                                                const QString& userId,
                                                int sex,
                                                bool preserveExistingIcon)
{
    const ProfileCurrentUserState snapshot = currentUserSnapshot(_state_port);
    QString nextIcon = normalizeIconForQml(icon);
    static const QString kDefaultIcon = QStringLiteral("qrc:/res/head_1.png");
    if (preserveExistingIcon && nextIcon == kDefaultIcon && snapshot.icon != kDefaultIcon)
    {
        nextIcon = snapshot.icon;
    }

    const ProfileAppliedUserState applied{uid, name, nick, nextIcon, desc, userId, sex};

    const auto userMgr = _gateway ? _gateway->userMgr() : nullptr;
    if (userMgr)
    {
        auto userInfo = userMgr->GetUserInfo();
        if (userInfo)
        {
            if (uid > 0)
            {
                userInfo->_uid = uid;
            }
            userInfo->_name = name;
            userInfo->_nick = nick;
            userInfo->_icon = nextIcon;
            userInfo->_desc = desc;
            userInfo->_user_id = userId;
            userInfo->_sex = sex;
        }
        else if (uid > 0)
        {
            userMgr->SetUserInfo(std::make_shared<UserInfo>(uid, name, nick, nextIcon, sex, QString(), desc, userId));
        }
    }

    if (_state_port.syncCurrentUser)
    {
        _state_port.syncCurrentUser(applied);
    }
}

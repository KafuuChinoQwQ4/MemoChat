#include "ProfileController.h"
#include "ClientGateway.h"
#include "httpmgr.h"
#include "global.h"
#include <QJsonObject>
#include <QUrl>

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
    emit chooseAvatarRequested(source);
}

void ProfileController::saveProfile(const QString& nick, const QString& desc)
{
    emit saveProfileRequested(nick, desc);
}

void ProfileController::clearStatus()
{
    emit clearStatusRequested();
}

bool ProfileController::validateProfile(const QString& nick, const QString& desc, QString* errorText) const
{
    const QString trimmedNick = nick.trimmed();
    if (trimmedNick.isEmpty())
    {
        if (errorText)
        {
            *errorText = "昵称不能为空";
        }
        return false;
    }
    if (trimmedNick.size() > 32)
    {
        if (errorText)
        {
            *errorText = "昵称长度不能超过32";
        }
        return false;
    }
    if (desc.trimmed().size() > 200)
    {
        if (errorText)
        {
            *errorText = "签名长度不能超过200";
        }
        return false;
    }
    return true;
}

void ProfileController::sendSaveProfile(int uid,
                                        const QString& name,
                                        const QString& nick,
                                        const QString& desc,
                                        const QString& icon) const
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["name"] = name;
    payload["nick"] = nick.trimmed();
    payload["desc"] = desc.trimmed();
    payload["icon"] = icon;

    _gateway->httpMgr()->PostHttpReq(QUrl(gate_url_prefix + "/user_update_profile"),
                                     payload,
                                     ReqId::ID_UPDATE_PROFILE,
                                     Modules::SETTINGSMOD,
                                     QStringLiteral("profile"));
}

void ProfileController::syncStatus(const QString& text, bool isError)
{
    if (_status_text == text && _status_error == isError)
    {
        return;
    }

    _status_text = text;
    _status_error = isError;
    emit statusChanged();
}

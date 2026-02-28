#include "ProfileController.h"
#include "ClientGateway.h"
#include "httpmgr.h"
#include "global.h"
#include <QJsonObject>
#include <QUrl>

ProfileController::ProfileController(ClientGateway *gateway)
    : _gateway(gateway)
{
}

bool ProfileController::validateProfile(const QString &nick, const QString &desc, QString *errorText) const
{
    const QString trimmedNick = nick.trimmed();
    if (trimmedNick.isEmpty()) {
        if (errorText) {
            *errorText = "昵称不能为空";
        }
        return false;
    }
    if (trimmedNick.size() > 32) {
        if (errorText) {
            *errorText = "昵称长度不能超过32";
        }
        return false;
    }
    if (desc.trimmed().size() > 200) {
        if (errorText) {
            *errorText = "签名长度不能超过200";
        }
        return false;
    }
    return true;
}

void ProfileController::sendSaveProfile(int uid, const QString &name, const QString &nick,
                                        const QString &desc, const QString &icon) const
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["name"] = name;
    payload["nick"] = nick.trimmed();
    payload["desc"] = desc.trimmed();
    payload["icon"] = icon;

    _gateway->httpMgr()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_update_profile"), payload, ReqId::ID_UPDATE_PROFILE, Modules::SETTINGSMOD);
}

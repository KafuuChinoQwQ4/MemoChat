#include "ProfileRequestPayloads.h"

namespace memochat::profile_payload
{
bool validateProfile(const QString& nick, const QString& desc, QString* errorText)
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

QJsonObject
buildSaveProfilePayload(int uid, const QString& name, const QString& nick, const QString& desc, const QString& icon)
{
    QJsonObject payload;
    payload["uid"] = uid;
    payload["name"] = name;
    payload["nick"] = nick.trimmed();
    payload["desc"] = desc.trimmed();
    payload["icon"] = icon;
    return payload;
}
} // namespace memochat::profile_payload

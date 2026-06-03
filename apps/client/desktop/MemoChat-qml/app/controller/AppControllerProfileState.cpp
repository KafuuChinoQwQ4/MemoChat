#include "AppController.h"
#include "IconPathUtils.h"
#include "usermgr.h"

QString AppController::normalizeIconPath(QString icon) const
{
    return normalizeIconForQml(icon);
}

void AppController::applyCurrentUserProfile(const QJsonObject& profile, bool preserveExistingIcon)
{
    if (profile.isEmpty())
    {
        return;
    }

    applyCurrentUserProfile(profile.value(
        QStringLiteral("uid"))
            .toInt(_pending_login_state.uid),
        profile.value(QStringLiteral("name")).toString(_user_state.name),
                      profile.value(QStringLiteral("nick")).toString(_user_state.nick),
                                    profile.value(QStringLiteral("icon")).toString(),
                                                  profile.value(QStringLiteral("desc")).toString(_user_state.desc),
                                                                profile.value(
                                                                    QStringLiteral("user_id"))
                                                                        .toString(_user_state.userId),
                                                                    profile.value(QStringLiteral("sex")).toInt(0),
                                                                                  preserveExistingIcon);
}

void AppController::applyCurrentUserProfile(int uid,
                                            const QString& name,
                                            const QString& nick,
                                            const QString& icon,
                                            const QString& desc,
                                            const QString& userId,
                                            int sex,
                                            bool preserveExistingIcon)
{
    QString nextIcon = normalizeIconPath(icon);
    static const QString kDefaultIcon = QStringLiteral("qrc:/res/head_1.png");
    if (preserveExistingIcon && nextIcon == kDefaultIcon && _user_state.icon != kDefaultIcon)
    {
        nextIcon = _user_state.icon;
    }

    auto userInfo = _gateway.userMgr()->GetUserInfo();
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
        _gateway.userMgr()->SetUserInfo(
            std::make_shared<UserInfo>(uid, name, nick, nextIcon, sex, QString(), desc, userId));
    }

    bool changed = false;
    if (_user_state.name != name)
    {
        _user_state.name = name;
        changed = true;
    }
    if (_user_state.nick != nick)
    {
        _user_state.nick = nick;
        changed = true;
    }
    if (_user_state.icon != nextIcon)
    {
        _user_state.icon = nextIcon;
        changed = true;
    }
    if (_user_state.desc != desc)
    {
        _user_state.desc = desc;
        changed = true;
    }
    if (_user_state.userId != userId)
    {
        _user_state.userId = userId;
        changed = true;
    }

    if (changed)
    {
        syncShellViewModelState();
        emit currentUserChanged();
    }
}

#include "AppController.h"
#include "IconPathUtils.h"

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

    _features.profileController.applyCurrentUserProfile(profile, preserveExistingIcon);
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
    _features.profileController.applyCurrentUserProfile(uid, name, nick, icon, desc, userId, sex, preserveExistingIcon);
}

void AppController::syncCurrentUserProfileState(const ProfileAppliedUserState& user)
{
    if (_shell_state.syncCurrentUser(user))
    {
        syncShellViewModelState();
    }
}

#include "AppCoordinators.h"

#include "AppController.h"
#include "LocalFilePickerService.h"
#include "usermgr.h"

ProfileCoordinator::ProfileCoordinator(AppController& controller)
    : _app(controller)
{
}

void ProfileCoordinator::chooseAvatar(int source)
{
    auto selfInfo = _app._gateway.userMgr()->GetUserInfo();
    if (!selfInfo)
    {
        _app.setSettingsStatus("用户状态异常，请重新登录", true);
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
            _app.setSettingsStatus(errorText, true);
        }
        return;
    }

    _app._gateway.userMgr()->UpdateIcon(avatarUrl);
    const QString normalized = _app.normalizeIconPath(avatarUrl);
    if (_app._user_state.icon != normalized)
    {
        _app._user_state.icon = normalized;
        _app.syncShellViewModelState();
        emit _app.currentUserChanged();
    }
    _app.setSettingsStatus("已选择新头像，点击保存后同步", false);
}

void ProfileCoordinator::saveProfile(const QString& nick, const QString& desc)
{
    _app.saveProfile(nick, desc);
}

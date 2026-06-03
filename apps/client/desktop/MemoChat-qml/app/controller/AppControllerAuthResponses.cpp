#include "AppController.h"
#include "AppCoordinators.h"

void AppController::onLoginHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _session_coordinator->onLoginHttpFinished(id, res, err);
}

void AppController::onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _session_coordinator->onRegisterHttpFinished(id, res, err);
}

void AppController::onResetHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _session_coordinator->onResetHttpFinished(id, res, err);
}

void AppController::onRegisterCountdownTimeout()
{
    _session_coordinator->onRegisterCountdownTimeout();
}

void AppController::onRegisterCodeCooldownTimeout()
{
    _session_coordinator->onRegisterCodeCooldownTimeout();
}

void AppController::onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (id != ReqId::ID_UPDATE_PROFILE)
    {
        return;
    }

    if (err != ErrorCodes::SUCCESS)
    {
        setSettingsStatus("资料同步失败：网络错误", true);
        return;
    }

    QJsonObject obj;
    if (!_auth_controller.parseJson(res, obj))
    {
        setSettingsStatus("资料同步失败：响应解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        setSettingsStatus("资料同步失败", true);
        return;
    }

    const QString nick = obj.value("nick").toString(_user_state.nick);
    const QString desc = obj.value("desc").toString(_user_state.desc);
    const QString icon = normalizeIconPath(obj.value("icon").toString(_user_state.icon));

    _gateway.userMgr()->UpdateNickAndDesc(nick, desc);
    _gateway.userMgr()->UpdateIcon(icon);

    bool changed = false;
    if (_user_state.nick != nick)
    {
        _user_state.nick = nick;
        changed = true;
    }
    if (_user_state.desc != desc)
    {
        _user_state.desc = desc;
        changed = true;
    }
    if (_user_state.icon != icon)
    {
        _user_state.icon = icon;
        changed = true;
    }

    if (changed)
    {
        syncShellViewModelState();
        emit currentUserChanged();
    }
    setSettingsStatus("资料已同步", false);
}

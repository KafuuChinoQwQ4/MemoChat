#include "AppCoordinators.h"

#include "AppController.h"
#include "IChatTransport.h"

#include <QDateTime>

namespace
{
constexpr int kRegisterCodeCooldownSeconds = 60;
}

bool SessionAuthCoordinator::checkEmailValid(const QString& email)
{
    QString errorText;
    if (!_app._auth_controller.checkEmail(email, &errorText))
    {
        _app.setTip(errorText, true);
        return false;
    }
    return true;
}

bool SessionAuthCoordinator::checkPasswordValid(const QString& password)
{
    QString errorText;
    if (!_app._auth_controller.checkPassword(password, &errorText))
    {
        _app.setTip(errorText, true);
        return false;
    }
    return true;
}

bool SessionAuthCoordinator::checkUserValid(const QString& user)
{
    QString errorText;
    if (!_app._auth_controller.checkUser(user, &errorText))
    {
        _app.setTip(errorText, true);
        return false;
    }
    return true;
}

bool SessionAuthCoordinator::checkVerifyCodeValid(const QString& code)
{
    QString errorText;
    if (!_app._auth_controller.checkVerifyCode(code, &errorText))
    {
        _app.setTip(errorText, true);
        return false;
    }
    return true;
}

void SessionAuthCoordinator::login(const QString& email, const QString& password)
{
    if (!checkEmailValid(email) || !checkPasswordValid(password))
    {
        return;
    }

    _app._pending_login_state.uid = 0;
    _app._pending_login_state.token.clear();
    _app._pending_login_state.loginTicket.clear();
    _app._pending_login_state.traceId.clear();
    _app._chat_endpoint_state.endpoints.clear();
    _app._chat_endpoint_state.endpointIndex = -1;
    _app._chat_endpoint_state.serverName.clear();
    _app._chat_endpoint_state.loginStartedMs = QDateTime::currentMSecsSinceEpoch();
    _app._chat_endpoint_state.httpLoginFinishedMs = 0;
    _app._chat_endpoint_state.connectStartedMs = 0;
    _app._chat_endpoint_state.connectFinishedMs = 0;
    _app._chat_login_timeout_timer.stop();
    _app._chat_recovery_state.ignoreNextLoginDisconnect = true;
    _app._gateway.chatTransport()->CloseConnection();
    _app.setBusy(true);
    _app.setTip("", false);
    _app._auth_controller.sendLogin(email, password);
}

void SessionAuthCoordinator::requestRegisterCode(const QString& email)
{
    if (_app._shell_state.registerCodeRequestPending || _app._shell_state.registerCodeCooldownSeconds > 0)
    {
        return;
    }

    if (!checkEmailValid(email))
    {
        return;
    }
    _app.setRegisterCodeRequestPending(true);
    _app.setRegisterCodeCooldownSeconds(kRegisterCodeCooldownSeconds);
    _app._register_code_cooldown_timer.start(1000);
    _app.setBusy(true);
    _app._auth_controller.sendVerifyCode(email, Modules::REGISTERMOD);
}

void SessionAuthCoordinator::registerUser(const QString& user,
                                          const QString& email,
                                          const QString& password,
                                          const QString& confirm,
                                          const QString& verifyCode)
{
    if (!checkUserValid(user) || !checkEmailValid(email) || !checkPasswordValid(password))
    {
        return;
    }
    if (password != confirm)
    {
        _app.setTip("密码和确认密码不匹配", true);
        return;
    }
    if (!checkVerifyCodeValid(verifyCode))
    {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendRegister(user, email, password, confirm, verifyCode);
}

void SessionAuthCoordinator::requestResetCode(const QString& email)
{
    if (!checkEmailValid(email))
    {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendVerifyCode(email, Modules::RESETMOD);
}

void SessionAuthCoordinator::onRegisterCodeCooldownTimeout()
{
    if (_app._shell_state.registerCodeCooldownSeconds <= 0)
    {
        _app._register_code_cooldown_timer.stop();
        return;
    }

    _app.setRegisterCodeCooldownSeconds(_app._shell_state.registerCodeCooldownSeconds - 1);
    if (_app._shell_state.registerCodeCooldownSeconds <= 0)
    {
        _app._register_code_cooldown_timer.stop();
    }
}

void SessionAuthCoordinator::resetPassword(const QString& user,
                                           const QString& email,
                                           const QString& password,
                                           const QString& verifyCode)
{
    if (!checkUserValid(user) || !checkEmailValid(email) || !checkPasswordValid(password) ||
        !checkVerifyCodeValid(verifyCode))
    {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendResetPassword(user, email, password, verifyCode);
}

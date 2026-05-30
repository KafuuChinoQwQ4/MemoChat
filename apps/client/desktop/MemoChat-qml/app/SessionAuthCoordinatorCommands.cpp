#include "AppCoordinators.h"

#include "AppController.h"
#include "IChatTransport.h"

#include <QDateTime>

void SessionAuthCoordinator::login(const QString& email, const QString& password)
{
    if (!_app.checkEmailValid(email) || !_app.checkPwdValid(password))
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
    if (!_app.checkEmailValid(email))
    {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendVerifyCode(email, Modules::REGISTERMOD);
}

void SessionAuthCoordinator::registerUser(const QString& user,
                                          const QString& email,
                                          const QString& password,
                                          const QString& confirm,
                                          const QString& verifyCode)
{
    if (!_app.checkUserValid(user) || !_app.checkEmailValid(email) || !_app.checkPwdValid(password))
    {
        return;
    }
    if (password != confirm)
    {
        _app.setTip("密码和确认密码不匹配", true);
        return;
    }
    if (!_app.checkVerifyCodeValid(verifyCode))
    {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendRegister(user, email, password, confirm, verifyCode);
}

void SessionAuthCoordinator::requestResetCode(const QString& email)
{
    if (!_app.checkEmailValid(email))
    {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendVerifyCode(email, Modules::RESETMOD);
}

void SessionAuthCoordinator::resetPassword(const QString& user,
                                           const QString& email,
                                           const QString& password,
                                           const QString& verifyCode)
{
    if (!_app.checkUserValid(user) || !_app.checkEmailValid(email) || !_app.checkPwdValid(password) ||
        !_app.checkVerifyCodeValid(verifyCode))
    {
        return;
    }
    _app.setBusy(true);
    _app._auth_controller.sendResetPassword(user, email, password, verifyCode);
}

#include "AppCoordinators.h"

namespace
{
constexpr int kRegisterCodeCooldownSeconds = 60;
}

bool SessionAuthCoordinator::checkEmailValid(const QString& email)
{
    QString errorText;
    if (!_port.checkEmail(email, &errorText))
    {
        _port.setTip(errorText, true);
        return false;
    }
    return true;
}

bool SessionAuthCoordinator::checkPasswordValid(const QString& password)
{
    QString errorText;
    if (!_port.checkPassword(password, &errorText))
    {
        _port.setTip(errorText, true);
        return false;
    }
    return true;
}

bool SessionAuthCoordinator::checkUserValid(const QString& user)
{
    QString errorText;
    if (!_port.checkUser(user, &errorText))
    {
        _port.setTip(errorText, true);
        return false;
    }
    return true;
}

bool SessionAuthCoordinator::checkVerifyCodeValid(const QString& code)
{
    QString errorText;
    if (!_port.checkVerifyCode(code, &errorText))
    {
        _port.setTip(errorText, true);
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

    _port.prepareLoginAttempt();
    _port.setBusy(true);
    _port.setTip("", false);
    _port.sendLogin(email, password);
}

void SessionAuthCoordinator::requestRegisterCode(const QString& email)
{
    if (_port.registerCodeRequestPending() || _port.registerCodeCooldownSeconds() > 0)
    {
        return;
    }

    if (!checkEmailValid(email))
    {
        return;
    }
    _port.setRegisterCodeRequestPending(true);
    _port.setRegisterCodeCooldownSeconds(kRegisterCodeCooldownSeconds);
    startRegisterCodeCooldownTimer();
    _port.setBusy(true);
    _port.sendVerifyCode(email, Modules::REGISTERMOD);
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
        _port.setTip("密码和确认密码不匹配", true);
        return;
    }
    if (!checkVerifyCodeValid(verifyCode))
    {
        return;
    }
    _port.setBusy(true);
    _port.sendRegister(user, email, password, confirm, verifyCode);
}

void SessionAuthCoordinator::requestResetCode(const QString& email)
{
    if (!checkEmailValid(email))
    {
        return;
    }
    _port.setBusy(true);
    _port.sendVerifyCode(email, Modules::RESETMOD);
}

void SessionAuthCoordinator::onRegisterCodeCooldownTimeout()
{
    if (_port.registerCodeCooldownSeconds() <= 0)
    {
        stopRegisterCodeCooldownTimer();
        return;
    }

    _port.setRegisterCodeCooldownSeconds(_port.registerCodeCooldownSeconds() - 1);
    if (_port.registerCodeCooldownSeconds() <= 0)
    {
        stopRegisterCodeCooldownTimer();
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
    _port.setBusy(true);
    _port.sendResetPassword(user, email, password, verifyCode);
}

#include "AuthService.h"

AuthService::AuthService(ClientGateway* gateway)
    : _controller(gateway)
{
}

bool AuthService::parseJson(const QString& res, QJsonObject& obj) const
{
    return _controller.parseJson(res, obj);
}

bool AuthService::checkEmail(const QString& email, QString* errorText) const
{
    return _controller.checkEmail(email, errorText);
}

bool AuthService::checkPassword(const QString& password, QString* errorText) const
{
    return _controller.checkPassword(password, errorText);
}

bool AuthService::checkUser(const QString& user, QString* errorText) const
{
    return _controller.checkUser(user, errorText);
}

bool AuthService::checkVerifyCode(const QString& code, QString* errorText) const
{
    return _controller.checkVerifyCode(code, errorText);
}

void AuthService::sendLogin(const QString& email, const QString& password) const
{
    _controller.sendLogin(email, password);
}

void AuthService::sendVerifyCode(const QString& email, Modules module) const
{
    _controller.sendVerifyCode(email, module);
}

void AuthService::sendRegister(const QString& user,
                               const QString& email,
                               const QString& password,
                               const QString& confirm,
                               const QString& verifyCode) const
{
    _controller.sendRegister(user, email, password, confirm, verifyCode);
}

void AuthService::sendResetPassword(const QString& user,
                                    const QString& email,
                                    const QString& password,
                                    const QString& verifyCode) const
{
    _controller.sendResetPassword(user, email, password, verifyCode);
}

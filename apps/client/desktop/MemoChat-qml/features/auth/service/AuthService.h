#ifndef AUTHSERVICE_H
#define AUTHSERVICE_H

#include "AuthController.h"
#include "global.h"

#include <QJsonObject>
#include <QString>

class ClientGateway;

class AuthService
{
public:
    explicit AuthService(ClientGateway* gateway);

    bool parseJson(const QString& res, QJsonObject& obj) const;
    bool checkEmail(const QString& email, QString* errorText) const;
    bool checkPassword(const QString& password, QString* errorText) const;
    bool checkUser(const QString& user, QString* errorText) const;
    bool checkVerifyCode(const QString& code, QString* errorText) const;

    void sendLogin(const QString& email, const QString& password) const;
    void sendVerifyCode(const QString& email, Modules module) const;
    void sendRegister(const QString& user,
                      const QString& email,
                      const QString& password,
                      const QString& confirm,
                      const QString& verifyCode) const;
    void sendResetPassword(const QString& user,
                           const QString& email,
                           const QString& password,
                           const QString& verifyCode) const;

private:
    AuthController _controller;
};

#endif // AUTHSERVICE_H

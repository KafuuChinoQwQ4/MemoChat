#include "AuthController.h"
#include "ClientGateway.h"
#include "httpmgr.h"
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUrl>

AuthController::AuthController(ClientGateway *gateway)
    : _gateway(gateway)
{
}

bool AuthController::parseJson(const QString &res, QJsonObject &obj) const
{
    const QJsonDocument doc = QJsonDocument::fromJson(res.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }
    obj = doc.object();
    return true;
}

bool AuthController::checkEmail(const QString &email, QString *errorText) const
{
    const QRegularExpression regex(R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)");
    if (!regex.match(email.trimmed()).hasMatch()) {
        if (errorText) {
            *errorText = "邮箱地址不正确";
        }
        return false;
    }
    return true;
}

bool AuthController::checkPassword(const QString &password, QString *errorText) const
{
    if (password.length() < 6 || password.length() > 15) {
        if (errorText) {
            *errorText = "密码长度应为6~15";
        }
        return false;
    }

    const QRegularExpression regExp("^[a-zA-Z0-9!@#$%^&*.]{6,15}$");
    if (!regExp.match(password).hasMatch()) {
        if (errorText) {
            *errorText = "不能包含非法字符且长度为6~15";
        }
        return false;
    }
    return true;
}

bool AuthController::checkUser(const QString &user, QString *errorText) const
{
    if (user.trimmed().isEmpty()) {
        if (errorText) {
            *errorText = "用户名不能为空";
        }
        return false;
    }
    return true;
}

bool AuthController::checkVerifyCode(const QString &code, QString *errorText) const
{
    if (code.trimmed().isEmpty()) {
        if (errorText) {
            *errorText = "验证码不能为空";
        }
        return false;
    }
    return true;
}

void AuthController::sendLogin(const QString &email, const QString &password) const
{
    QJsonObject payload;
    payload["email"] = email.trimmed();
    payload["passwd"] = xorString(password);
    _gateway->httpMgr()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_login"), payload, ReqId::ID_LOGIN_USER, Modules::LOGINMOD);
}

void AuthController::sendVerifyCode(const QString &email, Modules module) const
{
    QJsonObject payload;
    payload["email"] = email.trimmed();
    _gateway->httpMgr()->PostHttpReq(
        QUrl(gate_url_prefix + "/get_varifycode"), payload, ReqId::ID_GET_VARIFY_CODE, module);
}

void AuthController::sendRegister(const QString &user, const QString &email, const QString &password,
                                  const QString &confirm, const QString &verifyCode) const
{
    QJsonObject payload;
    payload["user"] = user.trimmed();
    payload["email"] = email.trimmed();
    payload["passwd"] = xorString(password);
    payload["confirm"] = xorString(confirm);
    payload["varifycode"] = verifyCode.trimmed();
    payload["sex"] = 0;
    payload["icon"] = ":/res/head_1.jpg";
    payload["nick"] = user.trimmed();

    _gateway->httpMgr()->PostHttpReq(
        QUrl(gate_url_prefix + "/user_register"), payload, ReqId::ID_REG_USER, Modules::REGISTERMOD);
}

void AuthController::sendResetPassword(const QString &user, const QString &email, const QString &password,
                                       const QString &verifyCode) const
{
    QJsonObject payload;
    payload["user"] = user.trimmed();
    payload["email"] = email.trimmed();
    payload["passwd"] = xorString(password);
    payload["varifycode"] = verifyCode.trimmed();

    _gateway->httpMgr()->PostHttpReq(
        QUrl(gate_url_prefix + "/reset_pwd"), payload, ReqId::ID_RESET_PWD, Modules::RESETMOD);
}

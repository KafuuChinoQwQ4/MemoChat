#include "AppCoordinators.h"

#include <QJsonObject>

namespace
{
QString gateAuthBusinessErrorTip(int errorCode, const QJsonObject& obj)
{
    switch (errorCode)
    {
        case 0:
            return {};
        case 1:
            return QStringLiteral("服务器响应异常，请重试");
        case 1001:
            return QStringLiteral("请求参数格式错误");
        case 1002:
            return QStringLiteral("认证服务暂时不可用，请稍后重试");
        case 1003:
            return QStringLiteral("验证码已过期，请点击「获取」重新发送");
        case 1004:
            return QStringLiteral("验证码不正确");
        case 1005:
            return QStringLiteral("用户名或邮箱已被注册");
        case 1006:
            return QStringLiteral("密码校验失败，请检查两次密码是否一致");
        case 1007:
            return QStringLiteral("用户名与邮箱不匹配");
        case 1008:
            return QStringLiteral("密码更新失败，请稍后重试");
        case 1014:
        {
            const QString minVersion = obj.value(QStringLiteral("min_version")).toString(QStringLiteral("2.0.0"));
            return QStringLiteral("客户端版本过低，请升级到 %1 或以上").arg(minVersion);
        }
        case 1022:
            return QStringLiteral("邮件发送失败，请检查邮箱地址或稍后重试");
        default:
            return QStringLiteral("操作失败（错误码 %1）").arg(errorCode);
    }
}

} // namespace

void SessionAuthCoordinator::clearRegisterCodeRequestCooldown()
{
    _port.setRegisterCodeRequestPending(false);
    _port.setRegisterCodeCooldownSeconds(0);
    stopRegisterCodeCooldownTimer();
}

void SessionAuthCoordinator::onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS)
    {
        _port.setBusy(false);
        if (id == ReqId::ID_GET_VARIFY_CODE)
        {
            clearRegisterCodeRequestCooldown();
        }
        _port.setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!_port.parseJson(res, obj))
    {
        _port.setBusy(false);
        if (id == ReqId::ID_GET_VARIFY_CODE)
        {
            clearRegisterCodeRequestCooldown();
        }
        _port.setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        _port.setBusy(false);
        if (id == ReqId::ID_GET_VARIFY_CODE)
        {
            clearRegisterCodeRequestCooldown();
        }
        _port.setTip(gateAuthBusinessErrorTip(error, obj), true);
        return;
    }

    if (id == ReqId::ID_GET_VARIFY_CODE)
    {
        _port.setBusy(false);
        _port.setRegisterCodeRequestPending(false);
        _port.setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_REG_USER)
    {
        _port.setBusy(false);
        _port.setTip("用户注册成功", false);
        _port.setRegisterSuccessPage(true);
        _port.setRegisterCountdown(5);
        _port.startRegisterCountdown();
    }
}

void SessionAuthCoordinator::onResetHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS)
    {
        _port.setBusy(false);
        _port.setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!_port.parseJson(res, obj))
    {
        _port.setBusy(false);
        _port.setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        _port.setBusy(false);
        _port.setTip(gateAuthBusinessErrorTip(error, obj), true);
        return;
    }

    _port.setBusy(false);
    if (id == ReqId::ID_GET_VARIFY_CODE)
    {
        _port.setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_RESET_PWD)
    {
        _port.setTip("重置成功, 点击返回登录", false);
    }
}

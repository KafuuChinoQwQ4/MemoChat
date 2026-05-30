#include "AppCoordinators.h"

#include "AppController.h"

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
        default:
            return QStringLiteral("操作失败（错误码 %1）").arg(errorCode);
    }
}
} // namespace

void SessionAuthCoordinator::onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS)
    {
        _app.setBusy(false);
        _app.setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!_app.parseJson(res, obj))
    {
        _app.setBusy(false);
        _app.setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        _app.setBusy(false);
        _app.setTip(gateAuthBusinessErrorTip(error, obj), true);
        return;
    }

    if (id == ReqId::ID_GET_VARIFY_CODE)
    {
        _app.setBusy(false);
        _app.setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_REG_USER)
    {
        _app.setBusy(false);
        _app.setTip("用户注册成功", false);
        if (!_app._shell_state.registerSuccessPage)
        {
            _app._shell_state.registerSuccessPage = true;
            emit _app.registerSuccessPageChanged();
        }

        _app._shell_state.registerCountdown = 5;
        emit _app.registerCountdownChanged();
        _app._register_countdown_timer.start(1000);
    }
}

void SessionAuthCoordinator::onResetHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS)
    {
        _app.setBusy(false);
        _app.setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!_app.parseJson(res, obj))
    {
        _app.setBusy(false);
        _app.setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS)
    {
        _app.setBusy(false);
        _app.setTip(gateAuthBusinessErrorTip(error, obj), true);
        return;
    }

    _app.setBusy(false);
    if (id == ReqId::ID_GET_VARIFY_CODE)
    {
        _app.setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_RESET_PWD)
    {
        _app.setTip("重置成功, 点击返回登录", false);
    }
}

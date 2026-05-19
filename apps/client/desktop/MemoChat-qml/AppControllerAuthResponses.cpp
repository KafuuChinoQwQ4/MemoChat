#include "AppController.h"
#include "usermgr.h"

#include <QJsonObject>

namespace {
QString gateAuthBusinessErrorTip(int errorCode, const QJsonObject &obj)
{
    switch (errorCode) {
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
    case 1014: {
        const QString minVersion = obj.value(QStringLiteral("min_version")).toString(QStringLiteral("2.0.0"));
        return QStringLiteral("客户端版本过低，请升级到 %1 或以上").arg(minVersion);
    }
    default:
        return QStringLiteral("操作失败（错误码 %1）").arg(errorCode);
    }
}
}

void AppController::onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setBusy(false);
        setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip(gateAuthBusinessErrorTip(error, obj), true);
        return;
    }

    if (id == ReqId::ID_GET_VARIFY_CODE) {
        setBusy(false);
        setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_REG_USER) {
        setBusy(false);
        setTip("用户注册成功", false);
        if (!_register_success_page) {
            _register_success_page = true;
            emit registerSuccessPageChanged();
        }

        _register_countdown = 5;
        emit registerCountdownChanged();
        _register_countdown_timer.start(1000);
    }
}

void AppController::onResetHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (err != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip("网络请求错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setBusy(false);
        setTip("json解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setBusy(false);
        setTip(gateAuthBusinessErrorTip(error, obj), true);
        return;
    }

    setBusy(false);
    if (id == ReqId::ID_GET_VARIFY_CODE) {
        setTip("验证码已发送到邮箱，注意查收", false);
        return;
    }

    if (id == ReqId::ID_RESET_PWD) {
        setTip("重置成功, 点击返回登录", false);
    }
}

void AppController::onSettingsHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    if (id != ReqId::ID_UPDATE_PROFILE) {
        return;
    }

    if (err != ErrorCodes::SUCCESS) {
        setSettingsStatus("资料同步失败：网络错误", true);
        return;
    }

    QJsonObject obj;
    if (!parseJson(res, obj)) {
        setSettingsStatus("资料同步失败：响应解析错误", true);
        return;
    }

    const int error = obj.value("error").toInt(ErrorCodes::ERR_JSON);
    if (error != ErrorCodes::SUCCESS) {
        setSettingsStatus("资料同步失败", true);
        return;
    }

    const QString nick = obj.value("nick").toString(_current_user_nick);
    const QString desc = obj.value("desc").toString(_current_user_desc);
    const QString icon = normalizeIconPath(obj.value("icon").toString(_current_user_icon));

    _gateway.userMgr()->UpdateNickAndDesc(nick, desc);
    _gateway.userMgr()->UpdateIcon(icon);

    bool changed = false;
    if (_current_user_nick != nick) {
        _current_user_nick = nick;
        changed = true;
    }
    if (_current_user_desc != desc) {
        _current_user_desc = desc;
        changed = true;
    }
    if (_current_user_icon != icon) {
        _current_user_icon = icon;
        changed = true;
    }

    if (changed) {
        emit currentUserChanged();
    }
    setSettingsStatus("资料已同步", false);
}

#include "AppChatConnectionCoordinator.h"
#include "AppCoordinators.h"
#include "AppPortRegistry.h"
#include "IChatTransport.h"
#include "IconPathUtils.h"
#include "usermgr.h"

#include <QDateTime>
#include <QDebug>

SessionAuthPort AppPortRegistry::makeSessionAuthPort()
{
    return SessionAuthPort{
        [this](const QString& email, QString* errorText)
        {
            return _features.authController.checkEmail(email, errorText);
        },
        [this](const QString& password, QString* errorText)
        {
            return _features.authController.checkPassword(password, errorText);
        },
        [this](const QString& user, QString* errorText)
        {
            return _features.authController.checkUser(user, errorText);
        },
        [this](const QString& code, QString* errorText)
        {
            return _features.authController.checkVerifyCode(code, errorText);
        },
        [this](const QString& res, QJsonObject& obj)
        {
            return _features.authController.parseJson(res, obj);
        },
        [this](const QString& tip, bool isError)
        {
            setTip(tip, isError);
        },
        [this](bool value)
        {
            setBusy(value);
        },
        [this](const QString& email, const QString& password)
        {
            _features.authController.sendLogin(email, password);
        },
        [this](const QString& email, Modules module)
        {
            _features.authController.sendVerifyCode(email, module);
        },
        [this](const QString& user,
               const QString& email,
               const QString& password,
               const QString& confirm,
               const QString& verifyCode)
        {
            _features.authController.sendRegister(user, email, password, confirm, verifyCode);
        },
        [this](const QString& user, const QString& email, const QString& password, const QString& verifyCode)
        {
            _features.authController.sendResetPassword(user, email, password, verifyCode);
        },
        [this]()
        {
            return _features.authViewModel.registerCodeRequestPending();
        },
        [this]()
        {
            return _features.authViewModel.registerCodeCooldownSeconds();
        },
        [this](bool pending)
        {
            _features.authViewModel.syncRegisterCodeRequestPending(pending);
        },
        [this](int seconds)
        {
            _features.authViewModel.syncRegisterCodeCooldownSeconds(seconds);
        },
        [this](bool success)
        {
            _features.authViewModel.syncRegisterSuccessPage(success);
        },
        [this](int seconds)
        {
            _features.authViewModel.syncRegisterCountdown(seconds);
        },
        []() {},
        [this]()
        {
            _session_coordinator->resetLoginAttemptState(QDateTime::currentMSecsSinceEpoch());
            _shell_state.bootstrapState().postLoginBootstrapStarted = false;
            _shell_state.bootstrapState().chatLoginCompleted = false;
            _chat_login_timeout_timer.stop();
            _gateway.chatTransport()->CloseConnection();
        },
        [this](bool ignore)
        {
            _session_coordinator->setIgnoreNextLoginDisconnect(ignore);
        },
        [this](const ServerInfo& serverInfo, const QJsonObject& obj)
        {
            const QString traceId = obj.value(QStringLiteral("trace_id")).toString();
            _session_coordinator->applyLoginSuccessState(serverInfo, traceId, QDateTime::currentMSecsSinceEpoch());
            const AppPendingLoginState& pending = _session_coordinator->pendingLoginState();
            const AppChatEndpointState& endpointState = _session_coordinator->chatEndpointState();
            _gateway.userMgr()->SetToken(pending.token);
            _chat_login_timeout_timer.setInterval(endpointState.totalLoginTimeoutMs);
            _features.chatFeatureController.setMessageDownloadAuthContext(pending.uid, pending.token);
            setIconDownloadAuthContext(pending.uid, pending.token);
            applyCurrentUserProfile(obj.value(QStringLiteral("user_profile")).toObject(), false);
            _chat_connection_coordinator->resetReconnectState();
            setPage(_constants.chatPage);
            qInfo() << "HTTP login succeeded, connecting chat server host:" << endpointState.host
                    << "port:" << endpointState.port << "uid:" << pending.uid << "http_login_ms:"
                    << (endpointState.httpLoginFinishedMs > endpointState.loginStartedMs
                            ? (endpointState.httpLoginFinishedMs - endpointState.loginStartedMs)
                            : 0)
                    << "endpoint_count:" << endpointState.endpoints.size()
                    << "protocol_version:" << endpointState.protocolVersion;
            setTip("正在连接聊天服务...", false);
            _session_coordinator->setConnectStartedMs(QDateTime::currentMSecsSinceEpoch());
            _gateway.chatTransport()->connectToServer(serverInfo);
        }};
}

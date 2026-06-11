#include "AppCoordinators.h"

#include <utility>

AppSessionCoordinator::AppSessionCoordinator(SessionAuthPort authPort,
                                             PostLoginBootstrapPort chatEntryPort,
                                             RelationBootstrapPort relationBootstrapPort,
                                             RegisterCountdownPort registerCountdownPort,
                                             SessionLogoutPort logoutPort)
    : _logout_port(std::move(logoutPort))
{
    _register_countdown = std::make_unique<RegisterCountdownController>(std::move(registerCountdownPort));
    authPort.startRegisterCountdown = [this]()
    {
        _register_countdown->startTimer();
    };
    _auth = std::make_unique<SessionAuthCoordinator>(std::move(authPort));
    _chat_entry = std::make_unique<SessionChatEntryCoordinator>(std::move(chatEntryPort));
    _relation_bootstrap = std::make_unique<SessionRelationBootstrap>(std::move(relationBootstrapPort));
}

void AppSessionCoordinator::login(const QString& email, const QString& password)
{
    _auth->login(email, password);
}
void AppSessionCoordinator::requestRegisterCode(const QString& email)
{
    _auth->requestRegisterCode(email);
}
void AppSessionCoordinator::registerUser(const QString& user,
                                         const QString& email,
                                         const QString& password,
                                         const QString& confirm,
                                         const QString& verifyCode)
{
    _auth->registerUser(user, email, password, confirm, verifyCode);
}
void AppSessionCoordinator::requestResetCode(const QString& email)
{
    _auth->requestResetCode(email);
}
void AppSessionCoordinator::resetPassword(const QString& user,
                                          const QString& email,
                                          const QString& password,
                                          const QString& verifyCode)
{
    _auth->resetPassword(user, email, password, verifyCode);
}
void AppSessionCoordinator::onLoginHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _auth->onLoginHttpFinished(id, res, err);
}
void AppSessionCoordinator::onRegisterHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _auth->onRegisterHttpFinished(id, res, err);
}
void AppSessionCoordinator::onResetHttpFinished(ReqId id, QString res, ErrorCodes err)
{
    _auth->onResetHttpFinished(id, res, err);
}

void AppSessionCoordinator::onSwitchToChat()
{
    _chat_entry->onSwitchToChat();
}

void AppSessionCoordinator::beginPostLoginBootstrap()
{
    _chat_entry->beginPostLoginBootstrap();
}

void AppSessionCoordinator::runPostLoginBootstrap()
{
    _chat_entry->runPostLoginBootstrap();
}

void AppSessionCoordinator::requestRelationBootstrap()
{
    _relation_bootstrap->requestRelationBootstrap();
}

void AppSessionCoordinator::onRelationBootstrapUpdated()
{
    _relation_bootstrap->onRelationBootstrapUpdated();
}

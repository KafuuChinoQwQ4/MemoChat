#include "AppCoordinators.h"

#include "AppController.h"

AppSessionCoordinator::AppSessionCoordinator(AppController& controller)
    : _app(controller)
    , _auth(std::make_unique<SessionAuthCoordinator>(controller))
    , _chat_entry(std::make_unique<SessionChatEntryCoordinator>(controller))
    , _relation_bootstrap(std::make_unique<SessionRelationBootstrap>(controller))
    , _register_countdown(std::make_unique<RegisterCountdownController>(controller))
{
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

void AppSessionCoordinator::onRegisterCountdownTimeout()
{
    _register_countdown->onRegisterCountdownTimeout();
}

#include "AuthViewModel.h"

#include <QtGlobal>
#include <utility>

AuthViewModel::AuthViewModel(AuthService* service, QObject* parent)
    : QObject(parent)
    , _service(service)
    , _credentialStore()
{
    syncLoginCredentialCacheJson(_credentialStore.credentialCacheJson());
}

QString AuthViewModel::tipText() const
{
    return _state.tipText;
}

bool AuthViewModel::tipError() const
{
    return _state.tipError;
}

bool AuthViewModel::busy() const
{
    return _state.busy;
}

bool AuthViewModel::registerSuccessPage() const
{
    return _state.registerSuccessPage;
}

int AuthViewModel::registerCountdown() const
{
    return _state.registerCountdown;
}

int AuthViewModel::registerCodeCooldownSeconds() const
{
    return _state.registerCodeCooldownSeconds;
}

bool AuthViewModel::registerCodeRequestPending() const
{
    return _state.registerCodeRequestPending;
}

QString AuthViewModel::loginCredentialCacheJson() const
{
    return _loginCredentialCacheJson;
}

void AuthViewModel::syncTip(const QString& text, bool error)
{
    if (_state.tipText == text && _state.tipError == error)
    {
        return;
    }
    _state.tipText = text;
    _state.tipError = error;
    emit stateChanged();
}

void AuthViewModel::syncBusy(bool busy)
{
    if (_state.busy == busy)
    {
        return;
    }
    _state.busy = busy;
    emit stateChanged();
}

void AuthViewModel::syncRegisterSuccessPage(bool success)
{
    if (_state.registerSuccessPage == success)
    {
        return;
    }
    _state.registerSuccessPage = success;
    emit stateChanged();
}

void AuthViewModel::syncRegisterCountdown(int seconds)
{
    if (_state.registerCountdown == seconds)
    {
        return;
    }
    _state.registerCountdown = seconds;
    emit stateChanged();
}

void AuthViewModel::syncRegisterCodeCooldownSeconds(int seconds)
{
    const int normalized = qMax(0, seconds);
    if (_state.registerCodeCooldownSeconds == normalized)
    {
        return;
    }
    _state.registerCodeCooldownSeconds = normalized;
    emit stateChanged();
}

void AuthViewModel::syncRegisterCodeRequestPending(bool pending)
{
    if (_state.registerCodeRequestPending == pending)
    {
        return;
    }
    _state.registerCodeRequestPending = pending;
    emit stateChanged();
}

void AuthViewModel::syncLoginCredentialCacheJson(const QString& json)
{
    const QString normalized = json.trimmed().isEmpty() ? QStringLiteral("[]") : json;
    if (_loginCredentialCacheJson == normalized)
    {
        return;
    }
    _loginCredentialCacheJson = normalized;
    emit loginCredentialCacheChanged();
}

void AuthViewModel::setCommandPort(AuthCommandPort port)
{
    _command_port = std::move(port);
}

void AuthViewModel::clearTip()
{
    if (_command_port.clearTip)
    {
        _command_port.clearTip();
    }
}

void AuthViewModel::saveLoginCredential(const QString& email, const QString& password)
{
    Q_UNUSED(password)
    _credentialStore.saveLoginCredential(email, QString());
    syncLoginCredentialCacheJson(_credentialStore.credentialCacheJson());
}

void AuthViewModel::login(const QString& email, const QString& password)
{
    Q_UNUSED(_service)
    saveLoginCredential(email, password);
    if (_command_port.login)
    {
        _command_port.login(email, password);
    }
}

void AuthViewModel::requestRegisterCode(const QString& email)
{
    if (_command_port.requestRegisterCode)
    {
        _command_port.requestRegisterCode(email);
    }
}

void AuthViewModel::registerUser(const QString& user,
                                 const QString& email,
                                 const QString& password,
                                 const QString& confirm,
                                 const QString& verifyCode)
{
    if (_command_port.registerUser)
    {
        _command_port.registerUser(user, email, password, confirm, verifyCode);
    }
}

void AuthViewModel::requestResetCode(const QString& email)
{
    if (_command_port.requestResetCode)
    {
        _command_port.requestResetCode(email);
    }
}

void AuthViewModel::resetPassword(const QString& user,
                                  const QString& email,
                                  const QString& password,
                                  const QString& verifyCode)
{
    if (_command_port.resetPassword)
    {
        _command_port.resetPassword(user, email, password, verifyCode);
    }
}

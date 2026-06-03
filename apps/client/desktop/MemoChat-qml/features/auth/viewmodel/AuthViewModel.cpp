#include "AuthViewModel.h"

#include "AuthService.h"

#include <QtGlobal>

AuthViewModel::AuthViewModel(AuthService* service, QObject* parent)
    : QObject(parent)
    , _service(service)
{
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

void AuthViewModel::clearTip()
{
    emit clearTipRequested();
}

void AuthViewModel::saveLoginCredential(const QString& email, const QString& password)
{
    emit saveLoginCredentialRequested(email, password);
}

void AuthViewModel::login(const QString& email, const QString& password)
{
    Q_UNUSED(_service)
    emit loginRequested(email, password);
}

void AuthViewModel::requestRegisterCode(const QString& email)
{
    emit registerCodeRequested(email);
}

void AuthViewModel::registerUser(const QString& user,
                                 const QString& email,
                                 const QString& password,
                                 const QString& confirm,
                                 const QString& verifyCode)
{
    emit registerUserRequested(user, email, password, confirm, verifyCode);
}

void AuthViewModel::requestResetCode(const QString& email)
{
    emit resetCodeRequested(email);
}

void AuthViewModel::resetPassword(const QString& user,
                                  const QString& email,
                                  const QString& password,
                                  const QString& verifyCode)
{
    emit resetPasswordRequested(user, email, password, verifyCode);
}

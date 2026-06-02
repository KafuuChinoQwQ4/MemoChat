#include "AuthViewModel.h"

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

void AuthViewModel::clearTip()
{
    setTip(QString(), false);
}

void AuthViewModel::login(const QString& email, const QString& password)
{
    if (!_service)
    {
        setTip(QStringLiteral("Auth service is unavailable"), true);
        return;
    }

    QString errorText;
    if (!_service->validateEmail(email, &errorText) || !_service->validatePassword(password, &errorText))
    {
        setTip(errorText, true);
        return;
    }

    setBusy(true);
    _service->requestLogin(email, password);
}

void AuthViewModel::setTip(const QString& text, bool error)
{
    _state.tipText = text;
    _state.tipError = error;
    emit stateChanged();
}

void AuthViewModel::setBusy(bool busy)
{
    if (_state.busy == busy)
    {
        return;
    }
    _state.busy = busy;
    emit stateChanged();
}

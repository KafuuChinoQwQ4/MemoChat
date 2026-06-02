#include "AuthService.h"

#include "ClientGateway.h"

AuthService::AuthService(ClientGateway* gateway)
    : _gateway(gateway)
{
}

bool AuthService::validateEmail(const QString& email, QString* errorText) const
{
    if (email.trimmed().isEmpty())
    {
        if (errorText)
        {
            *errorText = QStringLiteral("Email is required");
        }
        return false;
    }
    return true;
}

bool AuthService::validatePassword(const QString& password, QString* errorText) const
{
    if (password.isEmpty())
    {
        if (errorText)
        {
            *errorText = QStringLiteral("Password is required");
        }
        return false;
    }
    return true;
}

void AuthService::requestLogin(const QString& email, const QString& password) const
{
    Q_UNUSED(email)
    Q_UNUSED(password)
    // Real module: call ClientGateway/HttpMgr and map the response into AuthViewModel.
}

void AuthService::requestRegisterCode(const QString& email) const
{
    Q_UNUSED(email)
}

void AuthService::requestResetCode(const QString& email) const
{
    Q_UNUSED(email)
}

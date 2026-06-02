#ifndef MEMOCHAT_TEMPLATE_AUTHSERVICE_H
#define MEMOCHAT_TEMPLATE_AUTHSERVICE_H

#include <QString>

class ClientGateway;

class AuthService
{
public:
    explicit AuthService(ClientGateway* gateway);

    bool validateEmail(const QString& email, QString* errorText) const;
    bool validatePassword(const QString& password, QString* errorText) const;

    void requestLogin(const QString& email, const QString& password) const;
    void requestRegisterCode(const QString& email) const;
    void requestResetCode(const QString& email) const;

private:
    ClientGateway* _gateway = nullptr;
};

#endif // MEMOCHAT_TEMPLATE_AUTHSERVICE_H

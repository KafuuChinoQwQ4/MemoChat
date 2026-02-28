#ifndef PROFILECONTROLLER_H
#define PROFILECONTROLLER_H

#include <QString>

class ClientGateway;

class ProfileController
{
public:
    explicit ProfileController(ClientGateway *gateway);

    bool validateProfile(const QString &nick, const QString &desc, QString *errorText) const;
    void sendSaveProfile(int uid, const QString &name, const QString &nick,
                         const QString &desc, const QString &icon) const;

private:
    ClientGateway *_gateway;
};

#endif // PROFILECONTROLLER_H

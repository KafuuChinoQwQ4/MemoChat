#ifndef CONTACTCONTROLLER_H
#define CONTACTCONTROLLER_H

#include <QVariantList>
#include <QString>

class ClientGateway;

class ContactController
{
public:
    explicit ContactController(ClientGateway *gateway);

    bool sendSearchUser(const QString &uidText, QString *errorText) const;
    void sendAddFriend(int selfUid, const QString &selfName, int targetUid,
                       const QString &bakName, const QVariantList &labels) const;
    void sendApproveFriend(int selfUid, int targetUid, const QString &remark,
                           const QVariantList &labels) const;

private:
    ClientGateway *_gateway;
};

#endif // CONTACTCONTROLLER_H

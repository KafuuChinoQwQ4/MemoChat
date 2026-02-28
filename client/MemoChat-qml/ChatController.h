#ifndef CHATCONTROLLER_H
#define CHATCONTROLLER_H

#include <QString>
#include "ChatDispatchService.h"

class ClientGateway;

class ChatController
{
public:
    explicit ChatController(ClientGateway *gateway);

    bool dispatchChatText(const OutgoingChatPacket &packet, QString *errorText) const;
    QString buildCallJoinUrl(int selfUid, int peerUid, const QString &callType) const;

private:
    ClientGateway *_gateway;
};

#endif // CHATCONTROLLER_H

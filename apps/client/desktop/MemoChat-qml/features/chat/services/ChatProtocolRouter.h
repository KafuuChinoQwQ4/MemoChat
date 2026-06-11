#ifndef CHATPROTOCOLROUTER_H
#define CHATPROTOCOLROUTER_H

#include "global.h"

#include <QJsonObject>

namespace memochat::chat
{
enum class GroupResponseRoute
{
    Ignore,
    Management,
    History,
    GroupMessageMutation,
    PrivateMessageMutation,
    PrivateForward,
    GroupMessageAck,
    DialogMeta
};

enum class MessageStatusRoute
{
    Ignore,
    PrivateMessageStatus,
    GroupMessageStatus
};

enum class GroupMemberEventRoute
{
    GroupManagement,
    GroupReadAck,
    GroupMessageMutation
};

class ChatProtocolRouter
{
public:
    static GroupResponseRoute routeGroupResponse(ReqId reqId);
    static MessageStatusRoute routeMessageStatus(const QJsonObject& payload);
    static GroupMemberEventRoute routeGroupMemberEvent(const QJsonObject& payload);
};
} // namespace memochat::chat

#endif // CHATPROTOCOLROUTER_H

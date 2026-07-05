#pragma once

#include "ports/IMessageCommandService.hpp"

class IDeliveryGateway;
class IMessageRepository;
class IRelationRepository;

class GroupMessageHistoryService
{
public:
    GroupMessageHistoryService(IMessageRepository* message_repository,
                               IRelationRepository* relation_repository,
                               IDeliveryGateway* delivery_gateway);

    MessageCommandResult GroupHistory(const MessageCommandRequest& request);
    MessageCommandResult GroupReadAck(const MessageCommandRequest& request);

private:
    IMessageRepository* _message_repository = nullptr;
    IRelationRepository* _relation_repository = nullptr;
    IDeliveryGateway* _delivery_gateway = nullptr;
};

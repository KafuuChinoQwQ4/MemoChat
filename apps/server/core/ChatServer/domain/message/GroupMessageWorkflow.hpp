#pragma once

#include "ports/IMessageCommandService.hpp"

class IDeliveryGateway;
class IEventPublisher;
class IMessageRepository;
class IRelationRepository;

class GroupMessageWorkflow
{
public:
    GroupMessageWorkflow(IMessageRepository* message_repository,
                         IRelationRepository* relation_repository,
                         IDeliveryGateway* delivery_gateway,
                         IEventPublisher* event_publisher);

    MessageCommandResult GroupChatMessage(const MessageCommandRequest& request);
    MessageCommandResult EditGroupMessage(const MessageCommandRequest& request);
    MessageCommandResult RevokeGroupMessage(const MessageCommandRequest& request);
    MessageCommandResult ForwardGroupMessage(const MessageCommandRequest& request);

private:
    IMessageRepository* _message_repository = nullptr;
    IRelationRepository* _relation_repository = nullptr;
    IDeliveryGateway* _delivery_gateway = nullptr;
    IEventPublisher* _event_publisher = nullptr;
};

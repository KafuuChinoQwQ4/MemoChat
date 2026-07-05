#pragma once

#include "ports/IMessageCommandService.hpp"

class IDeliveryGateway;
class IRelationRepository;

class GroupManagementService
{
public:
    GroupManagementService(IRelationRepository* relation_repository, IDeliveryGateway* delivery_gateway);

    MessageCommandResult UpdateGroupAnnouncement(const MessageCommandRequest& request);
    MessageCommandResult UpdateGroupIcon(const MessageCommandRequest& request);
    MessageCommandResult SetGroupAdmin(const MessageCommandRequest& request);
    MessageCommandResult MuteGroupMember(const MessageCommandRequest& request);
    MessageCommandResult KickGroupMember(const MessageCommandRequest& request);
    MessageCommandResult QuitGroup(const MessageCommandRequest& request);
    MessageCommandResult DissolveGroup(const MessageCommandRequest& request);

private:
    IRelationRepository* _relation_repository = nullptr;
    IDeliveryGateway* _delivery_gateway = nullptr;
};

#pragma once

#include "ports/IMessageCommandService.hpp"

class IDeliveryGateway;
class IRelationRepository;

class GroupMembershipService
{
public:
    GroupMembershipService(IRelationRepository* relation_repository, IDeliveryGateway* delivery_gateway);

    void BuildGroupListJson(int uid, memochat::json::JsonValue& out);
    MessageCommandResult CreateGroup(const MessageCommandRequest& request);
    MessageCommandResult GetGroupList(const MessageCommandRequest& request);
    MessageCommandResult InviteGroupMember(const MessageCommandRequest& request);
    MessageCommandResult ApplyJoinGroup(const MessageCommandRequest& request);
    MessageCommandResult ReviewGroupApply(const MessageCommandRequest& request);

private:
    IRelationRepository* _relation_repository = nullptr;
    IDeliveryGateway* _delivery_gateway = nullptr;
};

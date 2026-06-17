#pragma once

#include "ports/IDeliveryGateway.h"
#include "ports/IDeliveryTaskPublisher.h"
#include "ports/IEventPublisher.h"
#include "ports/IRelationCommandService.h"
#include "ports/IRelationBootstrapCache.h"
#include "ports/IRelationRepository.h"
#include "ports/IRelationService.h"

#include <memory>
#include <string>
#include "json/GlazeCompat.h"

class ChatRelationService : public IRelationService
{
public:
    ChatRelationService(IRelationRepository* relation_repository,
                        IRelationBootstrapCache* relation_bootstrap_cache,
                        IDeliveryGateway* delivery_gateway,
                        IDeliveryTaskPublisher* task_publisher,
                        IEventPublisher* event_publisher);

    void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override;
    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override;
    RelationCommandResult SearchUser(const RelationCommandRequest& request) override;
    RelationCommandResult AddFriendApply(const RelationCommandRequest& request) override;
    RelationCommandResult AuthFriendApply(const RelationCommandRequest& request) override;
    RelationCommandResult DeleteFriend(const RelationCommandRequest& request) override;
    RelationCommandResult GetDialogList(const RelationCommandRequest& request) override;
    RelationCommandResult SyncDraft(const RelationCommandRequest& request) override;
    RelationCommandResult PinDialog(const RelationCommandRequest& request) override;
    RelationCommandResult FilterFriendUids(const RelationCommandRequest& request) override;

private:
    IRelationRepository* _relation_repository = nullptr;
    IRelationBootstrapCache* _relation_bootstrap_cache = nullptr;
    IDeliveryGateway* _delivery_gateway = nullptr;
    IDeliveryTaskPublisher* _task_publisher = nullptr;
    IEventPublisher* _event_publisher = nullptr;
};

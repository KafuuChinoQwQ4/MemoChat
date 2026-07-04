#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "json/GlazeCompat.hpp"
#include "ports/IDeliveryGateway.hpp"
#include "ports/IEventPublisher.hpp"
#include "ports/IMessageRepository.hpp"
#include "ports/IOnlineRouteStore.hpp"
#include "ports/IRelationBootstrapCache.hpp"
#include "ports/IRelationRepository.hpp"
#include "ports/ISessionRegistry.hpp"

class IAsyncEventBus;

class AsyncEventDispatcher : public IEventPublisher
{
public:
    using StopRequested = std::function<bool()>;

    AsyncEventDispatcher(std::shared_ptr<IAsyncEventBus> event_bus,
                         StopRequested stop_requested,
                         IDeliveryGateway* delivery_gateway,
                         IMessageRepository* message_repository,
                         IRelationRepository* relation_repository,
                         ISessionRegistry* session_registry,
                         IOnlineRouteStore* online_route_store,
                         IRelationBootstrapCache* relation_bootstrap_cache);

    bool PublishEvent(const std::string& topic,
                      const memochat::json::JsonValue& payload,
                      std::string* error = nullptr) override;
    bool
    PublishAsyncEvent(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error = nullptr);
    void DealAsyncEvents();
    void HandlePrivateAsyncEvent(const memochat::json::JsonValue& root);
    void HandleGroupAsyncEvent(const memochat::json::JsonValue& root);
    void HandleDialogSyncEvent(const memochat::json::JsonValue& root);
    void HandleRelationStateEvent(const memochat::json::JsonValue& root);
    void NotifyMessageStatus(const memochat::json::JsonValue& payload);

private:
    std::shared_ptr<IAsyncEventBus> _event_bus;
    StopRequested _stop_requested;
    IDeliveryGateway* _delivery_gateway = nullptr;
    IMessageRepository* _message_repository = nullptr;
    IRelationRepository* _relation_repository = nullptr;
    ISessionRegistry* _session_registry = nullptr;
    IOnlineRouteStore* _online_route_store = nullptr;
    IRelationBootstrapCache* _relation_bootstrap_cache = nullptr;
};

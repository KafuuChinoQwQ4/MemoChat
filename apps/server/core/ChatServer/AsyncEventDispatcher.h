#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include "json/GlazeCompat.h"

class IAsyncEventBus;

class AsyncEventDispatcher {
public:
    using StopRequested = std::function<bool()>;
    using PushGroupPayloadFn = std::function<void(const std::vector<int>&, short, const memochat::json::JsonValue&, int)>;

    AsyncEventDispatcher(std::shared_ptr<IAsyncEventBus> event_bus, StopRequested stop_requested, PushGroupPayloadFn push_group_payload);

    bool PublishAsyncEvent(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error = nullptr);
    void DealAsyncEvents();
    void HandlePrivateAsyncEvent(const memochat::json::JsonValue& root);
    void HandleGroupAsyncEvent(const memochat::json::JsonValue& root);
    void HandleDialogSyncEvent(const memochat::json::JsonValue& root);
    void HandleRelationStateEvent(const memochat::json::JsonValue& root);
    void NotifyMessageStatus(const memochat::json::JsonValue& payload);

private:
    std::shared_ptr<IAsyncEventBus> _event_bus;
    StopRequested _stop_requested;
    PushGroupPayloadFn _push_group_payload;
};

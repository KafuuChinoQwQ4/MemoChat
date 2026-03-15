#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace Json {
class Value;
}

class IAsyncEventBus;

class AsyncEventDispatcher {
public:
    using StopRequested = std::function<bool()>;
    using PushGroupPayloadFn = std::function<void(const std::vector<int>&, short, const Json::Value&, int)>;

    AsyncEventDispatcher(std::shared_ptr<IAsyncEventBus> event_bus, StopRequested stop_requested, PushGroupPayloadFn push_group_payload);

    bool PublishAsyncEvent(const std::string& topic, const Json::Value& payload, std::string* error = nullptr);
    void DealAsyncEvents();
    void HandlePrivateAsyncEvent(const Json::Value& root);
    void HandleGroupAsyncEvent(const Json::Value& root);
    void HandleDialogSyncEvent(const Json::Value& root);
    void HandleRelationStateEvent(const Json::Value& root);
    void NotifyMessageStatus(const Json::Value& payload);

private:
    std::shared_ptr<IAsyncEventBus> _event_bus;
    StopRequested _stop_requested;
    PushGroupPayloadFn _push_group_payload;
};

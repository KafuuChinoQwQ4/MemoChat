#pragma once

#include <functional>
#include <string>
#include <vector>
namespace Json {
class Value;
}

class AsyncEventDispatcher {
public:
    using StopRequested = std::function<bool()>;
    using PushGroupPayloadFn = std::function<void(const std::vector<int>&, short, const Json::Value&, int)>;

    AsyncEventDispatcher(StopRequested stop_requested, PushGroupPayloadFn push_group_payload);

    bool PublishAsyncEvent(const std::string& topic, const Json::Value& payload, std::string* error = nullptr);
    void DealAsyncEvents();
    void HandlePrivateAsyncEvent(const Json::Value& root);
    void HandleGroupAsyncEvent(const Json::Value& root);
    void NotifyMessageStatus(const Json::Value& payload);

private:
    StopRequested _stop_requested;
    PushGroupPayloadFn _push_group_payload;
};

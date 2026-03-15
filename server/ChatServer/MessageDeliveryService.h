#pragma once

#include <string>
#include <functional>
#include <vector>

namespace Json {
class Value;
}

class MessageDeliveryService {
public:
    using PublishTaskFn = std::function<bool(const std::string&, const std::string&, const Json::Value&, int, int, std::string*)>;

    explicit MessageDeliveryService(PublishTaskFn publish_task = nullptr);

    void PushPayload(const std::vector<int>& recipients, short msgid, const Json::Value& payload, int exclude_uid = 0);
    bool TryPushPayload(const std::vector<int>& recipients, short msgid, const Json::Value& payload, int exclude_uid = 0, bool enqueue_on_failure = false);

private:
    PublishTaskFn _publish_task;
};

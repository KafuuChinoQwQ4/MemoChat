#pragma once

#include "json/GlazeCompat.h"
#include <string>
#include <functional>
#include <vector>

class MessageDeliveryService {
public:
    using PublishTaskFn = std::function<bool(const std::string&, const std::string&, const memochat::json::JsonValue&, int, int, std::string*)>;

    explicit MessageDeliveryService(PublishTaskFn publish_task = nullptr);

    void PushPayload(const std::vector<int>& recipients, short msgid, const memochat::json::JsonValue& payload, int exclude_uid = 0);
    bool TryPushPayload(const std::vector<int>& recipients, short msgid, const memochat::json::JsonValue& payload, int exclude_uid = 0, bool enqueue_on_failure = false);

private:
    PublishTaskFn _publish_task;
};

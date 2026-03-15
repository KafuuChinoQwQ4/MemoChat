#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Json {
class Value;
}

class IAsyncTaskBus;

class TaskDispatcher {
public:
    using StopRequested = std::function<bool()>;
    using DeliveryTaskHandler = std::function<bool(const Json::Value&, bool)>;
    using OutboxRepairHandler = std::function<bool(int64_t)>;

    TaskDispatcher(std::shared_ptr<IAsyncTaskBus> task_bus,
        StopRequested stop_requested,
        DeliveryTaskHandler delivery_handler,
        OutboxRepairHandler outbox_repair_handler);

    bool PublishTask(const std::string& task_type,
        const std::string& routing_key,
        const Json::Value& payload,
        int delay_ms,
        int max_retries,
        std::string* error = nullptr);
    void DealTasks();

private:
    bool HandleTask(const Json::Value& payload, const std::string& task_type);

    std::shared_ptr<IAsyncTaskBus> _task_bus;
    StopRequested _stop_requested;
    DeliveryTaskHandler _delivery_handler;
    OutboxRepairHandler _outbox_repair_handler;
};

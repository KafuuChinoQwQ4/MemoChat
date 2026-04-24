#pragma once

#include "json/GlazeCompat.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class IAsyncTaskBus;

class TaskDispatcher {
public:
    using StopRequested = std::function<bool()>;
    using DeliveryTaskHandler = std::function<bool(const memochat::json::JsonValue&, bool)>;
    using OutboxRepairHandler = std::function<bool(int64_t)>;

    TaskDispatcher(std::shared_ptr<IAsyncTaskBus> task_bus,
        StopRequested stop_requested,
        DeliveryTaskHandler delivery_handler,
        OutboxRepairHandler outbox_repair_handler);

    bool PublishTask(const std::string& task_type,
        const std::string& routing_key,
        const memochat::json::JsonValue& payload,
        int delay_ms,
        int max_retries,
        std::string* error = nullptr);
    void DealTasks();

private:
    bool HandleTask(const memochat::json::JsonValue& payload, const std::string& task_type);

    std::shared_ptr<IAsyncTaskBus> _task_bus;
    StopRequested _stop_requested;
    DeliveryTaskHandler _delivery_handler;
    OutboxRepairHandler _outbox_repair_handler;
};

#pragma once

#include "json/GlazeCompat.hpp"
#include "ports/IDeliveryGateway.hpp"
#include "ports/IDeliveryTaskPublisher.hpp"
#include "ports/IOutboxRepairScheduler.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class IAsyncTaskBus;

class TaskDispatcher : public IDeliveryTaskPublisher
{
public:
    using StopRequested = std::function<bool()>;

    TaskDispatcher(std::shared_ptr<IAsyncTaskBus> task_bus,
                   StopRequested stop_requested,
                   IDeliveryGateway* delivery_gateway,
                   IOutboxRepairScheduler* outbox_repair_scheduler);

    bool PublishTask(const std::string& task_type,
                     const std::string& routing_key,
                     const memochat::json::JsonValue& payload,
                     int delay_ms,
                     int max_retries,
                     std::string* error = nullptr);
    bool PublishDeliveryTask(const std::string& task_type,
                             const std::string& routing_key,
                             const memochat::json::JsonValue& payload,
                             int delay_ms,
                             int max_retries,
                             std::string* error = nullptr) override;
    void DealTasks();

private:
    bool HandleTask(const memochat::json::JsonValue& payload, const std::string& task_type);

    std::shared_ptr<IAsyncTaskBus> _task_bus;
    StopRequested _stop_requested;
    IDeliveryGateway* _delivery_gateway = nullptr;
    IOutboxRepairScheduler* _outbox_repair_scheduler = nullptr;
};

#include "TaskDispatcher.h"

#include "ChatRuntime.h"
#include "ChatTaskEnvelope.h"
#include "IAsyncTaskBus.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include <chrono>
#include "json/GlazeCompat.h"
#include <thread>

namespace {
const std::vector<std::string>& TaskRoutingKeys()
{
    static const std::vector<std::string> keys = {
        memochat::chatruntime::TaskRoutingDeliveryRetry(),
        memochat::chatruntime::TaskRoutingOfflineNotify(),
        memochat::chatruntime::TaskRoutingRelationNotify(),
        memochat::chatruntime::TaskRoutingOutboxRepair()
    };
    return keys;
}
}

TaskDispatcher::TaskDispatcher(std::shared_ptr<IAsyncTaskBus> task_bus,
    StopRequested stop_requested,
    DeliveryTaskHandler delivery_handler,
    OutboxRepairHandler outbox_repair_handler)
    : _task_bus(std::move(task_bus)),
      _stop_requested(std::move(stop_requested)),
      _delivery_handler(std::move(delivery_handler)),
      _outbox_repair_handler(std::move(outbox_repair_handler)) {
}

bool TaskDispatcher::PublishTask(const std::string& task_type,
    const std::string& routing_key,
    const memochat::json::JsonValue& payload,
    int delay_ms,
    int max_retries,
    std::string* error)
{
    if (!_task_bus) {
        if (error) {
            *error = "task_bus_unavailable";
        }
        return false;
    }
    auto envelope = BuildTaskEnvelope(task_type, routing_key, payload, delay_ms, max_retries);
    return _task_bus->Publish(envelope, error);
}

void TaskDispatcher::DealTasks()
{
    while (!_stop_requested()) {
        ConsumedTask task;
        std::string consume_error;
        const bool handled = _task_bus && _task_bus->ConsumeOnce(TaskRoutingKeys(), task, &consume_error);
        if (!handled) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        memolog::TraceContext::SetTraceId(task.envelope.trace_id);
        memolog::TraceContext::SetRequestId(task.envelope.request_id);
        const bool ok = HandleTask(task.envelope.payload, task.envelope.task_type);
        if (ok) {
            _task_bus->AckLastConsumed();
        } else {
            _task_bus->NackLastConsumed("task_handler_failed");
        }
        memolog::TraceContext::Clear();
    }
}

bool TaskDispatcher::HandleTask(const memochat::json::JsonValue& payload, const std::string& task_type)
{
    if (task_type == "message_delivery_retry" ||
        task_type == "offline_notify" ||
        task_type == "relation_notify") {
        return _delivery_handler && _delivery_handler(payload, false);
    }
    if (task_type == "outbox_repair") {
        const auto outbox_id = payload.get("outbox_id", 0).asInt64();
        return _outbox_repair_handler && outbox_id > 0 && _outbox_repair_handler(outbox_id);
    }
    memolog::LogWarn("chat.task.unknown_type", "task type is not registered",
        {
            {"task_type", task_type}
        });
    return true;
}


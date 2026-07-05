#include "TaskDispatcher.hpp"

#include "ChatRuntime.hpp"
#include "ChatTaskEnvelope.hpp"
#include "IAsyncTaskBus.hpp"
#include "MessageDeliveryTaskPayload.hpp"
#include "logging/Logger.hpp"
#include "logging/TraceContext.hpp"

#include <chrono>
#include <exception>
#include "json/GlazeCompat.hpp"
#include <thread>

import memochat.chat.task_dispatcher_algorithms;

namespace task_dispatcher_modules = memochat::chat::delivery::task_dispatcher::modules;

namespace
{
const std::vector<std::string>& TaskRoutingKeys()
{
    static const std::vector<std::string> keys = {memochat::chatruntime::TaskRoutingDeliveryRetry(),
                                                  memochat::chatruntime::TaskRoutingOfflineNotify(),
                                                  memochat::chatruntime::TaskRoutingRelationNotify(),
                                                  memochat::chatruntime::TaskRoutingOutboxRepair()};
    return keys;
}
} // namespace

TaskDispatcher::TaskDispatcher(std::shared_ptr<IAsyncTaskBus> task_bus,
                               StopRequested stop_requested,
                               IDeliveryGateway* delivery_gateway,
                               IOutboxRepairScheduler* outbox_repair_scheduler)
    : _task_bus(std::move(task_bus))
    , _stop_requested(std::move(stop_requested))
    , _delivery_gateway(delivery_gateway)
    , _outbox_repair_scheduler(outbox_repair_scheduler)
{
}

bool TaskDispatcher::PublishTask(const std::string& task_type,
                                 const std::string& routing_key,
                                 const memochat::json::JsonValue& payload,
                                 int delay_ms,
                                 int max_retries,
                                 std::string* error)
{
    if (!task_dispatcher_modules::ShouldPublishTask(static_cast<bool>(_task_bus)))
    {
        if (error)
        {
            *error = task_dispatcher_modules::TaskBusUnavailableError();
        }
        return false;
    }
    auto envelope = BuildTaskEnvelope(task_type, routing_key, payload, delay_ms, max_retries);
    return _task_bus->Publish(envelope, error);
}

bool TaskDispatcher::PublishDeliveryTask(const std::string& task_type,
                                         const std::string& routing_key,
                                         const memochat::json::JsonValue& payload,
                                         int delay_ms,
                                         int max_retries,
                                         std::string* error)
{
    return PublishTask(task_type, routing_key, payload, delay_ms, max_retries, error);
}

void TaskDispatcher::DealTasks()
{
    while (!_stop_requested())
    {
        ConsumedTask task;
        std::string consume_error;
        bool handled = false;
        try
        {
            handled = _task_bus && _task_bus->ConsumeOnce(TaskRoutingKeys(), task, &consume_error);
        }
        catch (const std::bad_alloc& error)
        {
            memolog::LogError("chat.task.dispatcher.consume_bad_alloc",
                              "task dispatcher stopped after task bus allocation failure",
                              {{"error", error.what()}});
            break;
        }
        catch (const std::exception& error)
        {
            memolog::LogWarn("chat.task.dispatcher.consume_exception",
                             "task dispatcher caught task bus exception",
                             {{"error", error.what()}});
            if (_stop_requested())
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(task_dispatcher_modules::NoTaskSleepMs()));
            continue;
        }
        catch (...)
        {
            memolog::LogWarn("chat.task.dispatcher.consume_unknown_exception",
                             "task dispatcher caught unknown task bus exception");
            if (_stop_requested())
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(task_dispatcher_modules::NoTaskSleepMs()));
            continue;
        }
        if (!handled)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(task_dispatcher_modules::NoTaskSleepMs()));
            continue;
        }

        memolog::TraceContext::SetTraceId(task.envelope.trace_id);
        memolog::TraceContext::SetRequestId(task.envelope.request_id);
        const bool ok = HandleTask(task.envelope.payload, task.envelope.task_type);
        if (ok)
        {
            _task_bus->AckLastConsumed();
        }
        else
        {
            _task_bus->NackLastConsumed(task_dispatcher_modules::TaskHandlerFailedError());
        }
        memolog::TraceContext::Clear();
    }
}

bool TaskDispatcher::HandleTask(const memochat::json::JsonValue& payload, const std::string& task_type)
{
    if (task_dispatcher_modules::IsDeliveryTaskType(task_type.data(), task_type.size()))
    {
        memochat::chat::delivery::MessageDeliveryTaskPayload task_payload;
        if (!memochat::chat::delivery::ParseDeliveryTaskPayload(payload, &task_payload))
        {
            return task_dispatcher_modules::ShouldAckInvalidDeliveryTaskPayload();
        }
        return _delivery_gateway && _delivery_gateway->TryPushPayload({task_payload.recipient_uid},
                                                                      static_cast<short>(task_payload.msgid),
                                                                      task_payload.payload,
                                                                      task_payload.exclude_uid,
                                                                      false);
    }
    if (task_dispatcher_modules::IsOutboxRepairTaskType(task_type.data(), task_type.size()))
    {
        const auto outbox_id = payload.get("outbox_id", 0).asInt64();
        return task_dispatcher_modules::ShouldExpediteOutboxRepair(_outbox_repair_scheduler != nullptr, outbox_id) &&
               _outbox_repair_scheduler->ExpediteOutboxRepair(outbox_id);
    }
    memolog::LogWarn(task_dispatcher_modules::UnknownTaskTypeEventName(),
                     task_dispatcher_modules::UnknownTaskTypeMessage(),
                     {{"task_type", task_type}});
    return true;
}

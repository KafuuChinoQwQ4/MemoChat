#include "InlineTaskBus.h"

#include "logging/Logger.h"

#include <algorithm>
#include <chrono>

namespace {
int64_t NowMsInlineTask()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}
}

bool InlineTaskBus::Publish(const TaskEnvelope& task, std::string* error)
{
    (void)error;
    std::lock_guard<std::mutex> guard(_mutex);
    _queue.push_back(task);
    return true;
}

bool InlineTaskBus::ConsumeOnce(const std::vector<std::string>& routing_keys, ConsumedTask& task, std::string* error)
{
    (void)error;
    std::lock_guard<std::mutex> guard(_mutex);
    const auto now_ms = NowMsInlineTask();
    for (auto it = _queue.begin(); it != _queue.end(); ++it) {
        if (!RoutingKeyAccepted(routing_keys, *it) || it->available_at_ms > now_ms) {
            continue;
        }
        task = ConsumedTask();
        task.envelope = *it;
        task.serialized = SerializeTaskEnvelope(task.envelope);
        task.parsed = true;
        _last_consumed = task;
        _has_last = true;
        _queue.erase(it);
        return true;
    }
    return false;
}

void InlineTaskBus::AckLastConsumed()
{
    std::lock_guard<std::mutex> guard(_mutex);
    _last_consumed = ConsumedTask();
    _has_last = false;
}

void InlineTaskBus::NackLastConsumed(const std::string& error)
{
    std::lock_guard<std::mutex> guard(_mutex);
    if (!_has_last) {
        return;
    }
    TaskEnvelope task = _last_consumed.envelope;
    task.retry_count += 1;
    if (task.retry_count > task.max_retries) {
        memolog::LogWarn("chat.task.inline_drop", "inline task dropped after retries",
            {
                {"task_id", task.task_id},
                {"task_type", task.task_type},
                {"routing_key", task.routing_key},
                {"retry_count", std::to_string(task.retry_count)},
                {"error", error}
            });
    } else {
        task.available_at_ms = NowMsInlineTask() + 1000;
        _queue.push_back(task);
    }
    _last_consumed = ConsumedTask();
    _has_last = false;
}

bool InlineTaskBus::RoutingKeyAccepted(const std::vector<std::string>& routing_keys, const TaskEnvelope& task) const
{
    if (routing_keys.empty()) {
        return true;
    }
    return std::find(routing_keys.begin(), routing_keys.end(), task.routing_key) != routing_keys.end();
}

#include "ChatOutboxService.h"

#include "PostgresMgr.h"
#include "logging/Logger.h"

#include <algorithm>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>

namespace {
int64_t NowMsOutbox()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}
}

ChatOutboxService::ChatOutboxService(const KafkaConfig& config, PublishFn publish_fn, PublishRepairTaskFn publish_repair_task_fn)
    : _config(config),
      _publish_fn(std::move(publish_fn)),
      _publish_repair_task_fn(std::move(publish_repair_task_fn)) {
}

ChatOutboxService::~ChatOutboxService()
{
    Stop();
}

void ChatOutboxService::Start()
{
    if (_thread.joinable()) {
        return;
    }
    _stop = false;
    _thread = std::thread(&ChatOutboxService::RunLoop, this);
    memolog::LogInfo("chat.outbox.started", "chat outbox relay started",
        {
            {"batch_size", std::to_string(_config.outbox_batch_size)},
            {"retry_base_ms", std::to_string(_config.outbox_retry_base_ms)},
            {"retry_max_ms", std::to_string(_config.outbox_retry_max_ms)}
        });
}

void ChatOutboxService::Stop()
{
    _stop = true;
    if (_thread.joinable()) {
        _thread.join();
    }
    memolog::LogInfo("chat.outbox.stopped", "chat outbox relay stopped");
}

void ChatOutboxService::RunLoop()
{
    while (!_stop.load()) {
        std::vector<ChatOutboxEventInfo> events;
        if (!PostgresMgr::GetInstance()->GetPendingChatOutboxEvents(_config.outbox_batch_size, events) || events.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        for (const auto& event : events) {
            if (_stop.load()) {
                break;
            }
            std::string publish_error;
            const bool ok = _publish_fn && _publish_fn(event.topic, event.partition_key, event.payload_json, &publish_error);
            if (ok) {
                PostgresMgr::GetInstance()->MarkChatOutboxEventPublished(event.id, NowMsOutbox());
                memolog::LogInfo("chat.outbox.published", "chat outbox event published",
                    {
                        {"event_id", event.event_id},
                        {"topic", event.topic},
                        {"partition_key", event.partition_key},
                        {"retry_count", std::to_string(event.retry_count)}
                    });
                continue;
            }

            const int next_retry_count = event.retry_count + 1;
            const int backoff_ms = std::min(_config.outbox_retry_max_ms,
                _config.outbox_retry_base_ms * std::max(1, next_retry_count));
            const bool terminal_error = next_retry_count >= _config.consume_retry_max;
            PostgresMgr::GetInstance()->MarkChatOutboxEventRetry(
                event.id,
                next_retry_count,
                NowMsOutbox() + backoff_ms,
                publish_error,
                terminal_error);
            if (!terminal_error && _publish_repair_task_fn) {
                _publish_repair_task_fn(event.id, backoff_ms, _config.consume_retry_max, publish_error);
            }
            memolog::LogWarn("chat.outbox.publish_failed", "chat outbox event publish failed",
                {
                    {"event_id", event.event_id},
                    {"topic", event.topic},
                    {"partition_key", event.partition_key},
                    {"retry_count", std::to_string(next_retry_count)},
                    {"terminal_error", terminal_error ? "true" : "false"},
                    {"error", publish_error}
                });
        }
    }
}

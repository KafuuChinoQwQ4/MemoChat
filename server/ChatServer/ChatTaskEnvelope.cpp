#include "ChatTaskEnvelope.h"

#include "logging/TraceContext.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>
#include <chrono>
#include "json/GlazeCompat.h"
#include <memory>

namespace {
int64_t NowMsTaskEnvelope()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string NewTaskId()
{
    return boost::uuids::to_string(boost::uuids::random_generator()());
}
}

TaskEnvelope::TaskEnvelope()
    : payload(memochat::json::object_t{}) {
}

TaskEnvelope::TaskEnvelope(const TaskEnvelope& other)
    : task_id(other.task_id),
      task_type(other.task_type),
      trace_id(other.trace_id),
      request_id(other.request_id),
      created_at_ms(other.created_at_ms),
      available_at_ms(other.available_at_ms),
      retry_count(other.retry_count),
      max_retries(other.max_retries),
      routing_key(other.routing_key),
      payload(other.payload) {
}

TaskEnvelope& TaskEnvelope::operator=(const TaskEnvelope& other)
{
    if (this == &other) {
        return *this;
    }
    task_id = other.task_id;
    task_type = other.task_type;
    trace_id = other.trace_id;
    request_id = other.request_id;
    created_at_ms = other.created_at_ms;
    available_at_ms = other.available_at_ms;
    retry_count = other.retry_count;
    max_retries = other.max_retries;
    routing_key = other.routing_key;
    payload = other.payload;
    return *this;
}

TaskEnvelope::~TaskEnvelope() = default;

TaskEnvelope BuildTaskEnvelope(const std::string& task_type,
    const std::string& routing_key,
    const memochat::json::JsonValue& incoming_payload,
    int delay_ms,
    int max_retries)
{
    TaskEnvelope envelope;
    envelope.task_id = memochat::json::glaze_safe_get<std::string>(incoming_payload, "task_id", "");
    if (envelope.task_id.empty()) {
        envelope.task_id = NewTaskId();
    }
    envelope.task_type = task_type;
    envelope.trace_id = memolog::TraceContext::GetTraceId();
    if (envelope.trace_id.empty()) {
        envelope.trace_id = memolog::TraceContext::EnsureTraceId();
    }
    envelope.request_id = memolog::TraceContext::GetRequestId();
    if (envelope.request_id.empty()) {
        envelope.request_id = memolog::TraceContext::NewId();
    }
    envelope.created_at_ms = NowMsTaskEnvelope();
    envelope.available_at_ms = envelope.created_at_ms + std::max(0, delay_ms);
    envelope.max_retries = std::max(0, max_retries);
    envelope.routing_key = routing_key;
    envelope.payload = incoming_payload;
    return envelope;
}

bool ParseTaskEnvelope(const std::string& serialized, TaskEnvelope& envelope)
{
    memochat::json::JsonValue root;
    std::string errors;
    if (!memochat::json::glaze_parse(root, serialized, &errors) || !root.is_object()) {
        return false;
    }
    envelope.task_id = memochat::json::glaze_safe_get<std::string>(root, "task_id", "");
    envelope.task_type = memochat::json::glaze_safe_get<std::string>(root, "task_type", "");
    envelope.trace_id = memochat::json::glaze_safe_get<std::string>(root, "trace_id", "");
    envelope.request_id = memochat::json::glaze_safe_get<std::string>(root, "request_id", "");
    envelope.created_at_ms = memochat::json::glaze_safe_get<int64_t>(root, "created_at_ms", int64_t{0});
    envelope.available_at_ms = memochat::json::glaze_safe_get<int64_t>(root, "available_at_ms", int64_t{0});
    envelope.retry_count = memochat::json::glaze_safe_get<int>(root, "retry_count", 0);
    envelope.max_retries = memochat::json::glaze_safe_get<int>(root, "max_retries", 0);
    envelope.routing_key = memochat::json::glaze_safe_get<std::string>(root, "routing_key", "");
    envelope.payload = memochat::json::glaze_get(root, "payload", memochat::json::object_t{});
    return true;
}

std::string SerializeTaskEnvelope(const TaskEnvelope& envelope)
{
    memochat::json::JsonValue root(memochat::json::object_t{});
    root["task_id"] = envelope.task_id;
    root["task_type"] = envelope.task_type;
    root["trace_id"] = envelope.trace_id;
    root["request_id"] = envelope.request_id;
    root["created_at_ms"] = static_cast<int64_t>(envelope.created_at_ms);
    root["available_at_ms"] = static_cast<int64_t>(envelope.available_at_ms);
    root["retry_count"] = envelope.retry_count;
    root["max_retries"] = envelope.max_retries;
    root["routing_key"] = envelope.routing_key;
    root["payload"] = envelope.payload;

    return memochat::json::writeString(root);
}

#include "ChatAsyncEvent.h"

#include "ChatRuntime.h"

#include <algorithm>
#include "json/GlazeCompat.h"
#include <memory>
#include <utility>

namespace {

memochat::json::JsonValue CloneJson(const memochat::json::JsonValue& value)
{
    return memochat::json::JsonValue(value);
}
}

AsyncEventEnvelope::AsyncEventEnvelope()
    : payload(memochat::json::object_t{}) {
}

AsyncEventEnvelope::AsyncEventEnvelope(const AsyncEventEnvelope& other)
    : event_id(other.event_id),
      topic(other.topic),
      partition_key(other.partition_key),
      trace_id(other.trace_id),
      request_id(other.request_id),
      retry_count(other.retry_count),
      payload(other.payload) {
}

AsyncEventEnvelope& AsyncEventEnvelope::operator=(const AsyncEventEnvelope& other)
{
    if (this == &other) {
        return *this;
    }
    event_id = other.event_id;
    topic = other.topic;
    partition_key = other.partition_key;
    trace_id = other.trace_id;
    request_id = other.request_id;
    retry_count = other.retry_count;
    payload = other.payload;
    return *this;
}

AsyncEventEnvelope::~AsyncEventEnvelope() = default;

std::string BuildAsyncEventPartitionKey(const std::string& topic, const memochat::json::JsonValue& payload)
{
    if (topic == memochat::chatruntime::TopicPrivate()) {
        auto mk = memochat::json::make_member_ref(payload);
        const int from_uid = memochat::json::glaze_safe_get<int>(mk, "fromuid", 0);
        const int to_uid = memochat::json::glaze_safe_get<int>(mk, "touid", 0);
        const int uid_min = std::min(from_uid, to_uid);
        const int uid_max = std::max(from_uid, to_uid);
        return std::to_string(uid_min) + ":" + std::to_string(uid_max);
    }

    if (topic == memochat::chatruntime::TopicGroup()) {
        return std::to_string(memochat::json::make_member_ref(payload)["groupid"].asInt64());
    }

    return topic;
}

AsyncEventEnvelope BuildAsyncEventEnvelope(const std::string& topic, const memochat::json::JsonValue& payload)
{
    AsyncEventEnvelope envelope;
    envelope.topic = topic;
    auto mk = memochat::json::make_member_ref(payload);
    envelope.event_id = memochat::json::glaze_safe_get<std::string>(mk, "event_id", "");
    envelope.trace_id = memochat::json::glaze_safe_get<std::string>(mk, "trace_id", "");
    envelope.request_id = memochat::json::glaze_safe_get<std::string>(mk, "request_id", "");
    envelope.retry_count = memochat::json::glaze_safe_get<int>(mk, "retry_count", 0);
    envelope.partition_key = BuildAsyncEventPartitionKey(topic, payload);
    envelope.payload = payload;
    return envelope;
}

bool ParseAsyncEventEnvelope(const std::string& serialized, AsyncEventEnvelope& envelope)
{
    memochat::json::JsonValue root;
    std::string errors;
    if (!memochat::json::glaze_parse(root, serialized, &errors) || !root.is_object()) {
        return false;
    }

    auto mk = memochat::json::make_member_ref(root);
    envelope.event_id = memochat::json::glaze_safe_get<std::string>(mk, "event_id", "");
    envelope.topic = memochat::json::glaze_safe_get<std::string>(mk, "topic", "");
    envelope.partition_key = memochat::json::glaze_safe_get<std::string>(mk, "partition_key", "");
    envelope.trace_id = memochat::json::glaze_safe_get<std::string>(mk, "trace_id", "");
    envelope.request_id = memochat::json::glaze_safe_get<std::string>(mk, "request_id", "");
    envelope.retry_count = memochat::json::glaze_safe_get<int>(mk, "retry_count", 0);
    envelope.payload = mk["payload"].asValue();
    return true;
}

std::string SerializeAsyncEventEnvelope(const AsyncEventEnvelope& envelope)
{
    memochat::json::JsonValue root(memochat::json::object_t{});
    root["event_id"] = envelope.event_id;
    root["topic"] = envelope.topic;
    root["partition_key"] = envelope.partition_key;
    root["trace_id"] = envelope.trace_id;
    root["request_id"] = envelope.request_id;
    root["retry_count"] = envelope.retry_count;
    root["payload"] = envelope.payload;

    return memochat::json::writeString(root);
}

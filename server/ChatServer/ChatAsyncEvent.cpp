#include "ChatAsyncEvent.h"

#include "ChatRuntime.h"

#include <algorithm>
#include <json/json.h>
#include <memory>
#include <utility>

namespace {
std::string JsonString(const Json::Value& value, const char* key)
{
    return value.get(key, "").asString();
}

Json::Value CloneJson(const Json::Value& value)
{
    return Json::Value(value);
}
}

AsyncEventEnvelope::AsyncEventEnvelope()
    : payload_ptr(new Json::Value(Json::objectValue)) {
}

AsyncEventEnvelope::AsyncEventEnvelope(const AsyncEventEnvelope& other)
    : event_id(other.event_id),
      topic(other.topic),
      partition_key(other.partition_key),
      trace_id(other.trace_id),
      request_id(other.request_id),
      retry_count(other.retry_count),
      payload_ptr(new Json::Value(other.payload())) {
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
    *payload_ptr = other.payload();
    return *this;
}

AsyncEventEnvelope::~AsyncEventEnvelope()
{
    delete payload_ptr;
}

const Json::Value& AsyncEventEnvelope::payload() const
{
    return *payload_ptr;
}

Json::Value& AsyncEventEnvelope::payload()
{
    return *payload_ptr;
}

void AsyncEventEnvelope::setPayload(const Json::Value& value)
{
    *payload_ptr = value;
}

std::string BuildAsyncEventPartitionKey(const std::string& topic, const Json::Value& payload)
{
    if (topic == memochat::chatruntime::TopicPrivate()) {
        const int from_uid = payload.get("fromuid", 0).asInt();
        const int to_uid = payload.get("touid", 0).asInt();
        const int uid_min = std::min(from_uid, to_uid);
        const int uid_max = std::max(from_uid, to_uid);
        return std::to_string(uid_min) + ":" + std::to_string(uid_max);
    }

    if (topic == memochat::chatruntime::TopicGroup()) {
        return std::to_string(payload.get("groupid", 0).asInt64());
    }

    return topic;
}

AsyncEventEnvelope BuildAsyncEventEnvelope(const std::string& topic, const Json::Value& payload)
{
    AsyncEventEnvelope envelope;
    envelope.topic = topic;
    envelope.event_id = JsonString(payload, "event_id");
    envelope.trace_id = JsonString(payload, "trace_id");
    envelope.request_id = JsonString(payload, "request_id");
    envelope.retry_count = payload.get("retry_count", 0).asInt();
    envelope.partition_key = BuildAsyncEventPartitionKey(topic, payload);
    envelope.setPayload(payload);
    return envelope;
}

bool ParseAsyncEventEnvelope(const std::string& serialized, AsyncEventEnvelope& envelope)
{
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    std::string errors;
    if (!reader->parse(serialized.data(), serialized.data() + serialized.size(), &root, &errors) || !root.isObject()) {
        return false;
    }

    envelope.event_id = root.get("event_id", "").asString();
    envelope.topic = root.get("topic", "").asString();
    envelope.partition_key = root.get("partition_key", "").asString();
    envelope.trace_id = root.get("trace_id", "").asString();
    envelope.request_id = root.get("request_id", "").asString();
    envelope.retry_count = root.get("retry_count", 0).asInt();
    envelope.setPayload(root.get("payload", Json::Value(Json::objectValue)));
    return true;
}

std::string SerializeAsyncEventEnvelope(const AsyncEventEnvelope& envelope)
{
    Json::Value root(Json::objectValue);
    root["event_id"] = envelope.event_id;
    root["topic"] = envelope.topic;
    root["partition_key"] = envelope.partition_key;
    root["trace_id"] = envelope.trace_id;
    root["request_id"] = envelope.request_id;
    root["retry_count"] = envelope.retry_count;
    root["payload"] = CloneJson(envelope.payload());

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

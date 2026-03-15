#include "ChatTaskEnvelope.h"

#include "logging/TraceContext.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <algorithm>
#include <chrono>
#include <json/json.h>
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
    : payload_ptr(new Json::Value(Json::objectValue)) {
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
      payload_ptr(new Json::Value(other.payload())) {
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
    *payload_ptr = other.payload();
    return *this;
}

TaskEnvelope::~TaskEnvelope()
{
    delete payload_ptr;
}

const Json::Value& TaskEnvelope::payload() const
{
    return *payload_ptr;
}

Json::Value& TaskEnvelope::payload()
{
    return *payload_ptr;
}

void TaskEnvelope::setPayload(const Json::Value& value)
{
    *payload_ptr = value;
}

TaskEnvelope BuildTaskEnvelope(const std::string& task_type,
    const std::string& routing_key,
    const Json::Value& payload,
    int delay_ms,
    int max_retries)
{
    TaskEnvelope envelope;
    envelope.task_id = payload.get("task_id", "").asString();
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
    envelope.setPayload(payload);
    return envelope;
}

bool ParseTaskEnvelope(const std::string& serialized, TaskEnvelope& envelope)
{
    Json::CharReaderBuilder builder;
    std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    Json::Value root;
    std::string errors;
    if (!reader->parse(serialized.data(), serialized.data() + serialized.size(), &root, &errors) || !root.isObject()) {
        return false;
    }
    envelope.task_id = root.get("task_id", "").asString();
    envelope.task_type = root.get("task_type", "").asString();
    envelope.trace_id = root.get("trace_id", "").asString();
    envelope.request_id = root.get("request_id", "").asString();
    envelope.created_at_ms = root.get("created_at_ms", 0).asInt64();
    envelope.available_at_ms = root.get("available_at_ms", 0).asInt64();
    envelope.retry_count = root.get("retry_count", 0).asInt();
    envelope.max_retries = root.get("max_retries", 0).asInt();
    envelope.routing_key = root.get("routing_key", "").asString();
    envelope.setPayload(root.get("payload", Json::Value(Json::objectValue)));
    return true;
}

std::string SerializeTaskEnvelope(const TaskEnvelope& envelope)
{
    Json::Value root(Json::objectValue);
    root["task_id"] = envelope.task_id;
    root["task_type"] = envelope.task_type;
    root["trace_id"] = envelope.trace_id;
    root["request_id"] = envelope.request_id;
    root["created_at_ms"] = static_cast<Json::Int64>(envelope.created_at_ms);
    root["available_at_ms"] = static_cast<Json::Int64>(envelope.available_at_ms);
    root["retry_count"] = envelope.retry_count;
    root["max_retries"] = envelope.max_retries;
    root["routing_key"] = envelope.routing_key;
    root["payload"] = envelope.payload();

    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, root);
}

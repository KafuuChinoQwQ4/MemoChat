#include "ChatTaskEnvelope.hpp"

#include "json/TypedJsonCodec.hpp"
#include "logging/TraceContext.hpp"

#include <algorithm>
#include <chrono>
#include <memory>

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <glaze/glaze.hpp>

import memochat.chat.messaging_envelope_algorithms;

namespace
{
int64_t NowMsTaskEnvelope()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

std::string NewTaskId()
{
    return boost::uuids::to_string(boost::uuids::random_generator()());
}

struct TaskEnvelopeWireDto
{
    std::string task_id;
    std::string task_type;
    std::string trace_id;
    std::string request_id;
    int64_t created_at_ms = 0;
    int64_t available_at_ms = 0;
    int retry_count = 0;
    int max_retries = 0;
    std::string routing_key;
    glz::generic_json<> payload = memochat::json::object_t{};
};

TaskEnvelopeWireDto ToWireDto(const TaskEnvelope& envelope)
{
    TaskEnvelopeWireDto dto;
    dto.task_id = envelope.task_id;
    dto.task_type = envelope.task_type;
    dto.trace_id = envelope.trace_id;
    dto.request_id = envelope.request_id;
    dto.created_at_ms = envelope.created_at_ms;
    dto.available_at_ms = envelope.available_at_ms;
    dto.retry_count = envelope.retry_count;
    dto.max_retries = envelope.max_retries;
    dto.routing_key = envelope.routing_key;
    dto.payload = envelope.payload.impl();
    return dto;
}

void FromWireDto(const TaskEnvelopeWireDto& dto, TaskEnvelope& envelope)
{
    envelope.task_id = dto.task_id;
    envelope.task_type = dto.task_type;
    envelope.trace_id = dto.trace_id;
    envelope.request_id = dto.request_id;
    envelope.created_at_ms = dto.created_at_ms;
    envelope.available_at_ms = dto.available_at_ms;
    envelope.retry_count = dto.retry_count;
    envelope.max_retries = dto.max_retries;
    envelope.routing_key = dto.routing_key;
    envelope.payload = memochat::json::JsonValue(dto.payload);
}
} // namespace

TaskEnvelope::TaskEnvelope()
    : payload(memochat::json::object_t{})
{
}

TaskEnvelope::TaskEnvelope(const TaskEnvelope& other)
    : task_id(other.task_id)
    , task_type(other.task_type)
    , trace_id(other.trace_id)
    , request_id(other.request_id)
    , created_at_ms(other.created_at_ms)
    , available_at_ms(other.available_at_ms)
    , retry_count(other.retry_count)
    , max_retries(other.max_retries)
    , routing_key(other.routing_key)
    , payload(other.payload)
{
}

TaskEnvelope& TaskEnvelope::operator=(const TaskEnvelope& other)
{
    if (this == &other)
    {
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
    if (envelope.task_id.empty())
    {
        envelope.task_id = NewTaskId();
    }
    envelope.task_type = task_type;
    envelope.trace_id = memolog::TraceContext::GetTraceId();
    if (envelope.trace_id.empty())
    {
        envelope.trace_id = memolog::TraceContext::EnsureTraceId();
    }
    envelope.request_id = memolog::TraceContext::GetRequestId();
    if (envelope.request_id.empty())
    {
        envelope.request_id = memolog::TraceContext::NewId();
    }
    envelope.created_at_ms = NowMsTaskEnvelope();
    envelope.available_at_ms =
        envelope.created_at_ms + memochat::chat::messaging::envelope_modules::NonNegative(delay_ms);
    envelope.max_retries = memochat::chat::messaging::envelope_modules::NonNegative(max_retries);
    envelope.routing_key = routing_key;
    envelope.payload = incoming_payload;
    return envelope;
}

bool ParseTaskEnvelope(const std::string& serialized, TaskEnvelope& envelope)
{
    TaskEnvelopeWireDto dto;
    if (!memochat::json::ReadTypedJson(serialized, &dto))
    {
        return false;
    }
    FromWireDto(dto, envelope);
    return true;
}

std::string SerializeTaskEnvelope(const TaskEnvelope& envelope)
{
    return memochat::json::WriteTypedJsonOrEmptyObject(ToWireDto(envelope));
}

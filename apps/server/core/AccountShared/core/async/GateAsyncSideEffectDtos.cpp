#include "GateAsyncSideEffectDtos.h"

#include "json/TypedJsonCodec.h"

#include <glaze/glaze.hpp>

#include <utility>

namespace
{

struct GateKafkaEventEnvelopeWireDto
{
    std::string event_id;
    std::string topic;
    std::string partition_key;
    std::string event_type;
    std::string trace_id;
    std::string request_id;
    int64_t created_at_ms = 0;
    int retry_count = 0;
    glz::generic_json<> payload = memochat::json::object_t{};
};

struct GateRabbitTaskEnvelopeWireDto
{
    std::string task_id;
    std::string task_type;
    std::string trace_id;
    std::string request_id;
    int64_t created_at_ms = 0;
    int retry_count = 0;
    int max_retries = 0;
    std::string routing_key;
    glz::generic_json<> payload = memochat::json::object_t{};
};

GateKafkaEventEnvelopeWireDto ToWireDto(const gateasync::GateKafkaEventEnvelopeDto& envelope)
{
    GateKafkaEventEnvelopeWireDto dto;
    dto.event_id = envelope.event_id;
    dto.topic = envelope.topic;
    dto.partition_key = envelope.partition_key;
    dto.event_type = envelope.event_type;
    dto.trace_id = envelope.trace_id;
    dto.request_id = envelope.request_id;
    dto.created_at_ms = envelope.created_at_ms;
    dto.retry_count = envelope.retry_count;
    dto.payload = envelope.payload.impl();
    return dto;
}

GateRabbitTaskEnvelopeWireDto ToWireDto(const gateasync::GateRabbitTaskEnvelopeDto& envelope)
{
    GateRabbitTaskEnvelopeWireDto dto;
    dto.task_id = envelope.task_id;
    dto.task_type = envelope.task_type;
    dto.trace_id = envelope.trace_id;
    dto.request_id = envelope.request_id;
    dto.created_at_ms = envelope.created_at_ms;
    dto.retry_count = envelope.retry_count;
    dto.max_retries = envelope.max_retries;
    dto.routing_key = envelope.routing_key;
    dto.payload = envelope.payload.impl();
    return dto;
}

template <typename T> memochat::json::JsonValue TypedJsonToJsonValue(const T& value)
{
    std::string body;
    if (!memochat::json::WriteTypedJson(value, &body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, body) || !root.isObject())
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    return root;
}

} // namespace

namespace gateasync
{

GateUserProfileChangedPayloadDto BuildUserProfileChangedPayload(int uid,
                                                                const std::string& user_id,
                                                                const std::string& email,
                                                                const std::string& name,
                                                                const std::string& nick,
                                                                const std::string& icon,
                                                                int sex)
{
    GateUserProfileChangedPayloadDto payload;
    payload.uid = uid;
    payload.user_id = user_id;
    payload.email = email;
    payload.name = name;
    payload.nick = nick;
    payload.icon = icon;
    payload.sex = sex;
    return payload;
}

GateLoginAuditPayloadDto BuildLoginAuditPayload(int uid,
                                                const std::string& user_id,
                                                const std::string& email,
                                                const std::string& chat_server,
                                                const std::string& chat_host,
                                                const std::string& chat_port,
                                                bool login_cache_hit)
{
    GateLoginAuditPayloadDto payload;
    payload.uid = uid;
    payload.user_id = user_id;
    payload.email = email;
    payload.chat_server = chat_server;
    payload.chat_host = chat_host;
    payload.chat_port = chat_port;
    payload.login_cache_hit = login_cache_hit;
    return payload;
}

GateCacheInvalidatePayloadDto BuildCacheInvalidatePayload(const std::string& email,
                                                          const std::string& user_name,
                                                          const std::string& reason)
{
    GateCacheInvalidatePayloadDto payload;
    payload.email = email;
    payload.user_name = user_name;
    payload.reason = reason;
    payload.cache_domain = "login_profile";
    return payload;
}

GateKafkaEventEnvelopeDto BuildKafkaEventEnvelope(const std::string& event_id,
                                                  const std::string& topic,
                                                  const std::string& partition_key,
                                                  const std::string& event_type,
                                                  const std::string& trace_id,
                                                  const std::string& request_id,
                                                  int64_t created_at_ms,
                                                  int retry_count,
                                                  const memochat::json::JsonValue& payload)
{
    GateKafkaEventEnvelopeDto envelope;
    envelope.event_id = event_id;
    envelope.topic = topic;
    envelope.partition_key = partition_key;
    envelope.event_type = event_type;
    envelope.trace_id = trace_id;
    envelope.request_id = request_id;
    envelope.created_at_ms = created_at_ms;
    envelope.retry_count = retry_count;
    envelope.payload = payload;
    return envelope;
}

GateRabbitTaskEnvelopeDto BuildRabbitTaskEnvelope(const std::string& task_id,
                                                  const std::string& task_type,
                                                  const std::string& trace_id,
                                                  const std::string& request_id,
                                                  int64_t created_at_ms,
                                                  int retry_count,
                                                  int max_retries,
                                                  const std::string& routing_key,
                                                  const memochat::json::JsonValue& payload)
{
    GateRabbitTaskEnvelopeDto envelope;
    envelope.task_id = task_id;
    envelope.task_type = task_type;
    envelope.trace_id = trace_id;
    envelope.request_id = request_id;
    envelope.created_at_ms = created_at_ms;
    envelope.retry_count = retry_count;
    envelope.max_retries = max_retries;
    envelope.routing_key = routing_key;
    envelope.payload = payload;
    return envelope;
}

bool IsValidCacheInvalidatePayload(const GateCacheInvalidatePayloadDto& payload)
{
    return !payload.email.empty();
}

memochat::json::JsonValue ToJsonValue(const GateUserProfileChangedPayloadDto& payload)
{
    return TypedJsonToJsonValue(payload);
}

memochat::json::JsonValue ToJsonValue(const GateLoginAuditPayloadDto& payload)
{
    return TypedJsonToJsonValue(payload);
}

memochat::json::JsonValue ToJsonValue(const GateCacheInvalidatePayloadDto& payload)
{
    return TypedJsonToJsonValue(payload);
}

memochat::json::JsonValue ToJsonValue(const GateKafkaEventEnvelopeDto& envelope)
{
    return TypedJsonToJsonValue(ToWireDto(envelope));
}

memochat::json::JsonValue ToJsonValue(const GateRabbitTaskEnvelopeDto& envelope)
{
    return TypedJsonToJsonValue(ToWireDto(envelope));
}

bool EncodeGateKafkaEventEnvelope(const GateKafkaEventEnvelopeDto& envelope, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(ToWireDto(envelope), out, error_out);
}

bool EncodeGateRabbitTaskEnvelope(const GateRabbitTaskEnvelopeDto& envelope, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(ToWireDto(envelope), out, error_out);
}

bool DecodeCacheInvalidatePayload(const memochat::json::JsonValue& value,
                                  GateCacheInvalidatePayloadDto* out,
                                  std::string* error_out)
{
    const std::string body = memochat::json::glaze_stringify(value);
    return DecodeCacheInvalidatePayload(std::string_view(body), out, error_out);
}

bool DecodeCacheInvalidatePayload(std::string_view body, GateCacheInvalidatePayloadDto* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    GateCacheInvalidatePayloadDto parsed;
    if (!memochat::json::ReadTypedJson(body, &parsed, error_out))
    {
        return false;
    }
    if (!IsValidCacheInvalidatePayload(parsed))
    {
        return false;
    }
    *out = std::move(parsed);
    return true;
}

} // namespace gateasync

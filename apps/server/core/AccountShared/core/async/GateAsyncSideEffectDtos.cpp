#include "GateAsyncSideEffectDtos.hpp"

#include "json/TypedJsonCodec.hpp"

#include <glaze/glaze.hpp>

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

memochat::json::JsonValue ToJsonValue(const GateUserProfileChangedPayloadDto& payload)
{
    return TypedJsonToJsonValue(payload);
}

memochat::json::JsonValue ToJsonValue(const GateLoginAuditPayloadDto& payload)
{
    return TypedJsonToJsonValue(payload);
}

memochat::json::JsonValue ToJsonValue(const GateKafkaEventEnvelopeDto& envelope)
{
    return TypedJsonToJsonValue(ToWireDto(envelope));
}

bool EncodeGateKafkaEventEnvelope(const GateKafkaEventEnvelopeDto& envelope, std::string* out, std::string* error_out)
{
    return memochat::json::WriteTypedJson(ToWireDto(envelope), out, error_out);
}

} // namespace gateasync

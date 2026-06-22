#pragma once

#include "json/GlazeCompat.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace gateasync
{

struct GateUserProfileChangedPayloadDto
{
    int uid = 0;
    std::string user_id;
    std::string email;
    std::string name;
    std::string nick;
    std::string icon;
    int sex = 0;
};

struct GateLoginAuditPayloadDto
{
    int uid = 0;
    std::string user_id;
    std::string email;
    std::string chat_server;
    std::string chat_host;
    std::string chat_port;
    bool login_cache_hit = false;
};

struct GateCacheInvalidatePayloadDto
{
    std::string email;
    std::string user_name;
    std::string reason;
    std::string cache_domain;
};

struct GateKafkaEventEnvelopeDto
{
    std::string event_id;
    std::string topic;
    std::string partition_key;
    std::string event_type;
    std::string trace_id;
    std::string request_id;
    int64_t created_at_ms = 0;
    int retry_count = 0;
    memochat::json::JsonValue payload;
};

struct GateRabbitTaskEnvelopeDto
{
    std::string task_id;
    std::string task_type;
    std::string trace_id;
    std::string request_id;
    int64_t created_at_ms = 0;
    int retry_count = 0;
    int max_retries = 0;
    std::string routing_key;
    memochat::json::JsonValue payload;
};

GateUserProfileChangedPayloadDto BuildUserProfileChangedPayload(int uid,
                                                                const std::string& user_id,
                                                                const std::string& email,
                                                                const std::string& name,
                                                                const std::string& nick,
                                                                const std::string& icon,
                                                                int sex);

GateLoginAuditPayloadDto BuildLoginAuditPayload(int uid,
                                                const std::string& user_id,
                                                const std::string& email,
                                                const std::string& chat_server,
                                                const std::string& chat_host,
                                                const std::string& chat_port,
                                                bool login_cache_hit);

GateCacheInvalidatePayloadDto
BuildCacheInvalidatePayload(const std::string& email, const std::string& user_name, const std::string& reason);

GateKafkaEventEnvelopeDto BuildKafkaEventEnvelope(const std::string& event_id,
                                                  const std::string& topic,
                                                  const std::string& partition_key,
                                                  const std::string& event_type,
                                                  const std::string& trace_id,
                                                  const std::string& request_id,
                                                  int64_t created_at_ms,
                                                  int retry_count,
                                                  const memochat::json::JsonValue& payload);

GateRabbitTaskEnvelopeDto BuildRabbitTaskEnvelope(const std::string& task_id,
                                                  const std::string& task_type,
                                                  const std::string& trace_id,
                                                  const std::string& request_id,
                                                  int64_t created_at_ms,
                                                  int retry_count,
                                                  int max_retries,
                                                  const std::string& routing_key,
                                                  const memochat::json::JsonValue& payload);

bool IsValidCacheInvalidatePayload(const GateCacheInvalidatePayloadDto& payload);

memochat::json::JsonValue ToJsonValue(const GateUserProfileChangedPayloadDto& payload);
memochat::json::JsonValue ToJsonValue(const GateLoginAuditPayloadDto& payload);
memochat::json::JsonValue ToJsonValue(const GateCacheInvalidatePayloadDto& payload);
memochat::json::JsonValue ToJsonValue(const GateKafkaEventEnvelopeDto& envelope);
memochat::json::JsonValue ToJsonValue(const GateRabbitTaskEnvelopeDto& envelope);

bool EncodeGateKafkaEventEnvelope(const GateKafkaEventEnvelopeDto& envelope,
                                  std::string* out,
                                  std::string* error_out = nullptr);
bool EncodeGateRabbitTaskEnvelope(const GateRabbitTaskEnvelopeDto& envelope,
                                  std::string* out,
                                  std::string* error_out = nullptr);
bool DecodeCacheInvalidatePayload(const memochat::json::JsonValue& value,
                                  GateCacheInvalidatePayloadDto* out,
                                  std::string* error_out = nullptr);
bool DecodeCacheInvalidatePayload(std::string_view body,
                                  GateCacheInvalidatePayloadDto* out,
                                  std::string* error_out = nullptr);

} // namespace gateasync

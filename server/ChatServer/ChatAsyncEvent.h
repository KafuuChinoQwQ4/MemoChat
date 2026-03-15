#pragma once

#include <cstdint>
#include <string>

namespace Json {
class Value;
}

struct AsyncEventEnvelope {
    std::string event_id;
    std::string topic;
    std::string partition_key;
    std::string trace_id;
    std::string request_id;
    int retry_count = 0;
    Json::Value* payload_ptr = nullptr;

    AsyncEventEnvelope();
    AsyncEventEnvelope(const AsyncEventEnvelope& other);
    AsyncEventEnvelope& operator=(const AsyncEventEnvelope& other);
    ~AsyncEventEnvelope();

    const Json::Value& payload() const;
    Json::Value& payload();
    void setPayload(const Json::Value& value);
};

struct AsyncConsumedEvent {
    AsyncEventEnvelope envelope;
    std::string serialized;
    bool parsed = false;
};

struct AsyncOutboxRecord {
    int64_t id = 0;
    std::string event_id;
    std::string topic;
    std::string partition_key;
    std::string payload_json;
    int status = 0;
    int retry_count = 0;
    int64_t next_retry_at = 0;
    int64_t created_at = 0;
    int64_t published_at = 0;
    std::string last_error;
};

std::string BuildAsyncEventPartitionKey(const std::string& topic, const Json::Value& payload);
AsyncEventEnvelope BuildAsyncEventEnvelope(const std::string& topic, const Json::Value& payload);
bool ParseAsyncEventEnvelope(const std::string& serialized, AsyncEventEnvelope& envelope);
std::string SerializeAsyncEventEnvelope(const AsyncEventEnvelope& envelope);

#pragma once

#include <cstdint>
#include <string>
#include "json/GlazeCompat.h"

struct TaskEnvelope {
    std::string task_id;
    std::string task_type;
    std::string trace_id;
    std::string request_id;
    int64_t created_at_ms = 0;
    int64_t available_at_ms = 0;
    int retry_count = 0;
    int max_retries = 0;
    std::string routing_key;
    memochat::json::JsonValue payload;

    TaskEnvelope();
    TaskEnvelope(const TaskEnvelope& other);
    TaskEnvelope& operator=(const TaskEnvelope& other);
    ~TaskEnvelope();
};

struct ConsumedTask {
    TaskEnvelope envelope;
    std::string serialized;
    bool parsed = false;
};

TaskEnvelope BuildTaskEnvelope(const std::string& task_type,
    const std::string& routing_key,
    const memochat::json::JsonValue& payload,
    int delay_ms,
    int max_retries);
bool ParseTaskEnvelope(const std::string& serialized, TaskEnvelope& envelope);
std::string SerializeTaskEnvelope(const TaskEnvelope& envelope);

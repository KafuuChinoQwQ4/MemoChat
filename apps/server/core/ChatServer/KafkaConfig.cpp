#include "KafkaConfig.h"

#include "ChatRuntime.h"
#include "ConfigMgr.h"

#include <algorithm>
#include <cctype>

namespace {
int ReadIntConfig(const char* section, const char* key, int fallback)
{
    const auto raw = ConfigMgr::Inst().GetValue(section, key);
    if (raw.empty()) {
        return fallback;
    }
    try {
        return std::stoi(raw);
    } catch (...) {
        return fallback;
    }
}

bool ReadBoolConfig(const char* section, const char* key, bool fallback)
{
    const auto raw = ConfigMgr::Inst().GetValue(section, key);
    if (raw.empty()) {
        return fallback;
    }
    std::string value = raw;
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value == "1" || value == "true" || value == "yes" || value == "on";
}
}

bool KafkaConfig::valid() const
{
    return !brokers.empty() && !client_id.empty() && !consumer_group.empty();
}

KafkaConfig LoadKafkaConfig()
{
    KafkaConfig config;
    auto& cfg = ConfigMgr::Inst();
    config.brokers = cfg.GetValue("Kafka", "Brokers");
    config.client_id = cfg.GetValue("Kafka", "ClientId");
    config.consumer_group = cfg.GetValue("Kafka", "ConsumerGroup");
    config.topic_private = cfg.GetValue("Kafka", "TopicPrivate");
    config.topic_group = cfg.GetValue("Kafka", "TopicGroup");
    config.topic_private_dlq = cfg.GetValue("Kafka", "TopicPrivateDlq");
    config.topic_group_dlq = cfg.GetValue("Kafka", "TopicGroupDlq");
    config.enable_idempotence = ReadBoolConfig("Kafka", "EnableIdempotence", true);
    config.batch_num_messages = ReadIntConfig("Kafka", "BatchNumMessages", 1000);
    config.queue_buffering_max_ms = ReadIntConfig("Kafka", "QueueBufferingMaxMs", 10);
    config.session_timeout_ms = ReadIntConfig("Kafka", "SessionTimeoutMs", 10000);
    config.outbox_batch_size = ReadIntConfig("Kafka", "OutboxBatchSize", 100);
    config.outbox_retry_base_ms = ReadIntConfig("Kafka", "OutboxRetryBaseMs", 1000);
    config.outbox_retry_max_ms = ReadIntConfig("Kafka", "OutboxRetryMaxMs", 30000);
    config.consume_retry_max = ReadIntConfig("Kafka", "ConsumeRetryMax", 5);
    if (config.topic_private.empty()) {
        config.topic_private = memochat::chatruntime::TopicPrivate();
    }
    if (config.topic_group.empty()) {
        config.topic_group = memochat::chatruntime::TopicGroup();
    }
    if (config.topic_private_dlq.empty()) {
        config.topic_private_dlq = memochat::chatruntime::TopicPrivateDlq();
    }
    if (config.topic_group_dlq.empty()) {
        config.topic_group_dlq = memochat::chatruntime::TopicGroupDlq();
    }
    return config;
}

std::string DlqTopicFor(const KafkaConfig& config, const std::string& topic)
{
    if (topic == config.topic_private) {
        return config.topic_private_dlq;
    }
    if (topic == config.topic_group) {
        return config.topic_group_dlq;
    }
    return topic + ".dlq";
}

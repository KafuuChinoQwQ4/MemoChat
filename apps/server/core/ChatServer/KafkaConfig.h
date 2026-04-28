#pragma once

#include <string>

struct KafkaConfig {
    std::string brokers;
    std::string client_id;
    std::string consumer_group;
    std::string topic_private;
    std::string topic_group;
    std::string topic_private_dlq;
    std::string topic_group_dlq;
    bool enable_idempotence = true;
    int batch_num_messages = 1000;
    int queue_buffering_max_ms = 10;
    int session_timeout_ms = 10000;
    int outbox_batch_size = 100;
    int outbox_retry_base_ms = 1000;
    int outbox_retry_max_ms = 30000;
    int consume_retry_max = 5;

    bool valid() const;
};

KafkaConfig LoadKafkaConfig();
std::string DlqTopicFor(const KafkaConfig& config, const std::string& topic);

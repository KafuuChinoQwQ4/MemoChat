#pragma once

#include "ChatOutboxService.hpp"
#include "IAsyncEventBus.hpp"
#include "KafkaConfig.hpp"

#include <librdkafka/rdkafka.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class KafkaAsyncEventBus : public IAsyncEventBus
{
public:
    using PublishOutboxRepairTaskFn = std::function<bool(int64_t, int, int, std::string*)>;

    static bool BuildAvailable();

    explicit KafkaAsyncEventBus(PublishOutboxRepairTaskFn publish_outbox_repair_task = nullptr);
    ~KafkaAsyncEventBus() override;

    bool
    Publish(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error = nullptr) override;
    bool ConsumeOnce(const std::vector<std::string>& topics,
                     AsyncConsumedEvent& event,
                     std::string* error = nullptr) override;
    void AckLastConsumed() override;
    void NackLastConsumed(const std::string& error) override;

private:
    bool ProduceSerialized(const std::string& topic,
                           const std::string& partition_key,
                           const std::string& payload_json,
                           std::string* error);
    std::string DlqTopicForLastConsumed() const;
    void ClearLastConsumed();

    KafkaConfig _config;
    rd_kafka_t* _producer = nullptr;
    rd_kafka_t* _consumer = nullptr;
    std::unique_ptr<ChatOutboxService> _outbox_service;
    PublishOutboxRepairTaskFn _publish_outbox_repair_task;
    std::mutex _producer_mutex;
    std::mutex _consumer_mutex;
    rd_kafka_message_t* _last_message = nullptr;
    AsyncConsumedEvent _last_consumed;
};

#pragma once

#include "ChatOutboxService.h"
#include "IAsyncEventBus.h"
#include "KafkaConfig.h"

#include <memory>
#include <mutex>
#include <string>
#include <functional>

namespace cppkafka {
class Consumer;
class Producer;
class Message;
}

class KafkaAsyncEventBus : public IAsyncEventBus {
public:
    using PublishOutboxRepairTaskFn = std::function<bool(int64_t, int, int, std::string*)>;

    static bool BuildAvailable();

    explicit KafkaAsyncEventBus(PublishOutboxRepairTaskFn publish_outbox_repair_task = nullptr);
    ~KafkaAsyncEventBus() override;

    bool Publish(const std::string& topic, const Json::Value& payload, std::string* error = nullptr) override;
    bool ConsumeOnce(const std::vector<std::string>& topics, AsyncConsumedEvent& event, std::string* error = nullptr) override;
    void AckLastConsumed() override;
    void NackLastConsumed(const std::string& error) override;

private:
    bool ProduceSerialized(const std::string& topic, const std::string& partition_key, const std::string& payload_json, std::string* error);
    std::string DlqTopicForLastConsumed() const;
    void ClearLastConsumed();

    KafkaConfig _config;
    std::unique_ptr<cppkafka::Producer> _producer;
    std::unique_ptr<cppkafka::Consumer> _consumer;
    std::unique_ptr<ChatOutboxService> _outbox_service;
    PublishOutboxRepairTaskFn _publish_outbox_repair_task;
    std::mutex _producer_mutex;
    std::mutex _consumer_mutex;
    std::shared_ptr<cppkafka::Message> _last_message;
    AsyncConsumedEvent _last_consumed;
};

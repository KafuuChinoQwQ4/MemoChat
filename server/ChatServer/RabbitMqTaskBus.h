#pragma once

#include "IAsyncTaskBus.h"
#include "RabbitMqConfig.h"

#include <mutex>
#include <string>
#include <vector>

typedef struct amqp_connection_state_t_ *amqp_connection_state_t;

class RabbitMqTaskBus : public IAsyncTaskBus {
public:
    static bool BuildAvailable();

    RabbitMqTaskBus();
    ~RabbitMqTaskBus() override;

    bool Publish(const TaskEnvelope& task, std::string* error = nullptr) override;
    bool ConsumeOnce(const std::vector<std::string>& routing_keys, ConsumedTask& task, std::string* error = nullptr) override;
    void AckLastConsumed() override;
    void NackLastConsumed(const std::string& error) override;

private:
    bool Connect(std::string* error);
    void Close();
    bool EnsureTopology(std::string* error);
    bool PublishSerialized(const std::string& exchange, const std::string& routing_key, const std::string& payload, std::string* error);
    std::string QueueNameForRoutingKey(const std::string& routing_key) const;
    std::string RetryQueueNameForRoutingKey(const std::string& routing_key) const;
    std::string DlqRoutingKeyFor(const std::string& routing_key) const;
    void ClearLastConsumed();

    RabbitMqConfig _config;
    amqp_connection_state_t _connection = nullptr;
    std::mutex _mutex;
    std::vector<std::string> _subscribed_routing_keys;
    void* _last_envelope = nullptr;
    ConsumedTask _last_consumed;
};

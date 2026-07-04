import memochat.chat.runtime_composition_algorithms;

#include <string>

namespace modules = memochat::chat::orchestration::runtime_composition::modules;

bool MemoChatTestUseRabbitMqTaskBus(const std::string& backend, bool rabbitmq_available)
{
    return modules::ShouldUseRabbitMqTaskBus(backend.data(), backend.size(), rabbitmq_available);
}

bool MemoChatTestWarnRabbitMqTaskBusUnavailable(const std::string& backend, bool rabbitmq_available)
{
    return modules::ShouldWarnRabbitMqTaskBusUnavailable(backend.data(), backend.size(), rabbitmq_available);
}

bool MemoChatTestWarnUnsupportedTaskBusBackend(const std::string& backend)
{
    return modules::ShouldWarnUnsupportedTaskBusBackend(backend.data(), backend.size());
}

bool MemoChatTestUseKafkaAsyncEventBus(const std::string& backend, bool kafka_available)
{
    return modules::ShouldUseKafkaAsyncEventBus(backend.data(), backend.size(), kafka_available);
}

bool MemoChatTestWarnKafkaAsyncEventBusUnavailable(const std::string& backend, bool kafka_available)
{
    return modules::ShouldWarnKafkaAsyncEventBusUnavailable(backend.data(), backend.size(), kafka_available);
}

bool MemoChatTestWarnUnsupportedAsyncEventBusBackend(const std::string& backend)
{
    return modules::ShouldWarnUnsupportedAsyncEventBusBackend(backend.data(), backend.size());
}

std::string MemoChatTestInlineTaskBusBackend()
{
    return modules::InlineTaskBusBackend();
}

std::string MemoChatTestRedisAsyncEventBusBackend()
{
    return modules::RedisAsyncEventBusBackend();
}

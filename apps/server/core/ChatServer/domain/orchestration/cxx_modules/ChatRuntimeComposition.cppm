export module memochat.chat.runtime_composition_algorithms;

export namespace memochat::chat::orchestration::runtime_composition::modules
{
const char* RabbitMqTaskBusBackend()
{
    return "rabbitmq";
}

const char* InlineTaskBusBackend()
{
    return "inline";
}

const char* KafkaAsyncEventBusBackend()
{
    return "kafka";
}

const char* RedisAsyncEventBusBackend()
{
    return "redis";
}

bool EqualsLiteral(const char* data, unsigned long long size, const char* literal, unsigned long long literal_size)
{
    if (data == nullptr || literal == nullptr || size != literal_size)
    {
        return false;
    }
    for (unsigned long long index = 0; index < size; ++index)
    {
        if (data[index] != literal[index])
        {
            return false;
        }
    }
    return true;
}

bool IsRabbitMqTaskBusBackend(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "rabbitmq", 8);
}

bool IsInlineTaskBusBackend(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "inline", 6);
}

bool ShouldUseRabbitMqTaskBus(const char* data, unsigned long long size, bool rabbitmq_available)
{
    return IsRabbitMqTaskBusBackend(data, size) && rabbitmq_available;
}

bool ShouldWarnRabbitMqTaskBusUnavailable(const char* data, unsigned long long size, bool rabbitmq_available)
{
    return IsRabbitMqTaskBusBackend(data, size) && !rabbitmq_available;
}

bool ShouldWarnUnsupportedTaskBusBackend(const char* data, unsigned long long size)
{
    return !IsInlineTaskBusBackend(data, size) && !IsRabbitMqTaskBusBackend(data, size);
}

bool IsKafkaAsyncEventBusBackend(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "kafka", 5);
}

bool IsRedisAsyncEventBusBackend(const char* data, unsigned long long size)
{
    return EqualsLiteral(data, size, "redis", 5);
}

bool ShouldUseKafkaAsyncEventBus(const char* data, unsigned long long size, bool kafka_available)
{
    return IsKafkaAsyncEventBusBackend(data, size) && kafka_available;
}

bool ShouldWarnKafkaAsyncEventBusUnavailable(const char* data, unsigned long long size, bool kafka_available)
{
    return IsKafkaAsyncEventBusBackend(data, size) && !kafka_available;
}

bool ShouldWarnUnsupportedAsyncEventBusBackend(const char* data, unsigned long long size)
{
    return !IsRedisAsyncEventBusBackend(data, size) && !IsKafkaAsyncEventBusBackend(data, size);
}
} // namespace memochat::chat::orchestration::runtime_composition::modules

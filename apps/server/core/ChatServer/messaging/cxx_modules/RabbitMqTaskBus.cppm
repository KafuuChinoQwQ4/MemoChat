export module memochat.chat.rabbitmq_task_bus_algorithms;

export namespace memochat::chat::messaging::rabbitmq_task_bus::modules
{
bool BuildAvailable(bool compiled_with_rabbitmq)
{
    return compiled_with_rabbitmq;
}

bool ShouldRejectInvalidConfig(bool config_valid)
{
    return !config_valid;
}

bool ShouldRejectRpcReply(bool normal_reply)
{
    return !normal_reply;
}

bool ShouldReconnect(bool connection_missing)
{
    return connection_missing;
}

bool ShouldAckLastConsumed(bool connection_present, bool last_envelope_present)
{
    return connection_present && last_envelope_present;
}

bool ShouldNackLastConsumed(bool connection_present, bool last_envelope_present)
{
    return connection_present && last_envelope_present;
}

bool ShouldRouteToDeadLetter(int retry_count, int max_retries)
{
    return retry_count > max_retries;
}

bool ShouldRejectPublishResult(bool publish_ok)
{
    return !publish_ok;
}

int PersistentDeliveryMode()
{
    return 2;
}

const char* QueueSuffix()
{
    return ".q";
}

const char* RetryQueueSuffix()
{
    return ".retry";
}

const char* DlqSuffix()
{
    return ".dlq";
}

const char* ContentTypeJson()
{
    return "application/json";
}

const char* AmqpRpcReplyFailedError()
{
    return "amqp_rpc_reply_failed";
}

const char* RabbitMqBuildDisabledError()
{
    return "rabbitmq_build_disabled";
}

const char* RabbitMqInvalidConfigError()
{
    return "rabbitmq_invalid_config";
}

const char* RabbitMqSocketCreateFailedError()
{
    return "rabbitmq_socket_create_failed";
}

const char* RabbitMqSocketOpenFailedError()
{
    return "rabbitmq_socket_open_failed";
}

const char* RabbitMqPublishFailedError()
{
    return "rabbitmq_publish_failed";
}
} // namespace memochat::chat::messaging::rabbitmq_task_bus::modules

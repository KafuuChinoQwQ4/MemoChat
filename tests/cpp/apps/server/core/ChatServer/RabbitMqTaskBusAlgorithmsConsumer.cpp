import memochat.chat.rabbitmq_task_bus_algorithms;

bool MemoChatTestRabbitTaskBuildAvailable(bool compiled_with_rabbitmq)
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::BuildAvailable(compiled_with_rabbitmq);
}

bool MemoChatTestRabbitTaskShouldRejectInvalidConfig(bool config_valid)
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::ShouldRejectInvalidConfig(config_valid);
}

bool MemoChatTestRabbitTaskShouldRejectRpcReply(bool normal_reply)
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::ShouldRejectRpcReply(normal_reply);
}

bool MemoChatTestRabbitTaskShouldReconnect(bool connection_missing)
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::ShouldReconnect(connection_missing);
}

bool MemoChatTestRabbitTaskShouldAckLastConsumed(bool connection_present, bool last_envelope_present)
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::ShouldAckLastConsumed(connection_present,
                                                                                        last_envelope_present);
}

bool MemoChatTestRabbitTaskShouldNackLastConsumed(bool connection_present, bool last_envelope_present)
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::ShouldNackLastConsumed(connection_present,
                                                                                         last_envelope_present);
}

bool MemoChatTestRabbitTaskShouldRouteToDeadLetter(int retry_count, int max_retries)
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::ShouldRouteToDeadLetter(retry_count, max_retries);
}

bool MemoChatTestRabbitTaskShouldRejectPublishResult(bool publish_ok)
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::ShouldRejectPublishResult(publish_ok);
}

int MemoChatTestRabbitTaskPersistentDeliveryMode()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::PersistentDeliveryMode();
}

const char* MemoChatTestRabbitTaskQueueSuffix()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::QueueSuffix();
}

const char* MemoChatTestRabbitTaskRetryQueueSuffix()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::RetryQueueSuffix();
}

const char* MemoChatTestRabbitTaskDlqSuffix()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::DlqSuffix();
}

const char* MemoChatTestRabbitTaskContentTypeJson()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::ContentTypeJson();
}

const char* MemoChatTestRabbitTaskAmqpRpcReplyFailedError()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::AmqpRpcReplyFailedError();
}

const char* MemoChatTestRabbitTaskBuildDisabledError()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::RabbitMqBuildDisabledError();
}

const char* MemoChatTestRabbitTaskInvalidConfigError()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::RabbitMqInvalidConfigError();
}

const char* MemoChatTestRabbitTaskSocketCreateFailedError()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::RabbitMqSocketCreateFailedError();
}

const char* MemoChatTestRabbitTaskSocketOpenFailedError()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::RabbitMqSocketOpenFailedError();
}

const char* MemoChatTestRabbitTaskPublishFailedError()
{
    return memochat::chat::messaging::rabbitmq_task_bus::modules::RabbitMqPublishFailedError();
}

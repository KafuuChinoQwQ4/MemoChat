import memochat.chat.kafka_async_event_bus_algorithms;

bool MemoChatTestKafkaEventShouldStartFromValidConfig(bool config_valid)
{
    return memochat::chat::messaging::kafka_event_bus::modules::ShouldStartFromValidConfig(config_valid);
}

bool MemoChatTestKafkaEventShouldSkipOutboxRepairPublish(bool publish_callback_missing)
{
    return memochat::chat::messaging::kafka_event_bus::modules::ShouldSkipOutboxRepairPublish(publish_callback_missing);
}

bool MemoChatTestKafkaEventShouldRejectPublish(bool producer_missing, bool config_invalid)
{
    return memochat::chat::messaging::kafka_event_bus::modules::ShouldRejectPublish(producer_missing, config_invalid);
}

bool MemoChatTestKafkaEventShouldAssignGeneratedEventId(bool event_id_empty)
{
    return memochat::chat::messaging::kafka_event_bus::modules::ShouldAssignGeneratedEventId(event_id_empty);
}

bool MemoChatTestKafkaEventShouldRejectSerializedPayload(bool serialized_empty)
{
    return memochat::chat::messaging::kafka_event_bus::modules::ShouldRejectSerializedPayload(serialized_empty);
}

bool MemoChatTestKafkaEventShouldSetParseError(bool parsed, bool error_sink_present)
{
    return memochat::chat::messaging::kafka_event_bus::modules::ShouldSetParseError(parsed, error_sink_present);
}

bool MemoChatTestKafkaEventShouldCommitLastConsumed(bool consumer_present, bool last_message_present)
{
    return memochat::chat::messaging::kafka_event_bus::modules::ShouldCommitLastConsumed(consumer_present,
                                                                                         last_message_present);
}

bool MemoChatTestKafkaEventShouldNackLastConsumed(bool consumer_present, bool last_message_present)
{
    return memochat::chat::messaging::kafka_event_bus::modules::ShouldNackLastConsumed(consumer_present,
                                                                                       last_message_present);
}

bool MemoChatTestKafkaEventShouldRouteToDlq(int retry_count, int consume_retry_max)
{
    return memochat::chat::messaging::kafka_event_bus::modules::ShouldRouteToDlq(retry_count, consume_retry_max);
}

int MemoChatTestKafkaEventInitialOutboxStatus()
{
    return memochat::chat::messaging::kafka_event_bus::modules::InitialOutboxStatus();
}

int MemoChatTestKafkaEventInitialRetryCount()
{
    return memochat::chat::messaging::kafka_event_bus::modules::InitialRetryCount();
}

const char* MemoChatTestKafkaEventKafkaNotConfiguredError()
{
    return memochat::chat::messaging::kafka_event_bus::modules::KafkaNotConfiguredError();
}

const char* MemoChatTestKafkaEventSerializeFailedError()
{
    return memochat::chat::messaging::kafka_event_bus::modules::SerializeFailedError();
}

const char* MemoChatTestKafkaEventOutboxEnqueueFailedError()
{
    return memochat::chat::messaging::kafka_event_bus::modules::OutboxEnqueueFailedError();
}

const char* MemoChatTestKafkaEventKafkaBuildDisabledError()
{
    return memochat::chat::messaging::kafka_event_bus::modules::KafkaBuildDisabledError();
}

const char* MemoChatTestKafkaEventParseFailedError()
{
    return memochat::chat::messaging::kafka_event_bus::modules::ParseFailedError();
}

const char* MemoChatTestKafkaEventProducerUnavailableError()
{
    return memochat::chat::messaging::kafka_event_bus::modules::ProducerUnavailableError();
}

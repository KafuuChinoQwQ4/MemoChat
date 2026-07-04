export module memochat.chat.kafka_async_event_bus_algorithms;

export namespace memochat::chat::messaging::kafka_event_bus::modules
{
bool ShouldStartFromValidConfig(bool config_valid)
{
    return config_valid;
}

bool ShouldSkipOutboxRepairPublish(bool publish_callback_missing)
{
    return publish_callback_missing;
}

bool ShouldRejectPublish(bool producer_missing, bool config_invalid)
{
    return producer_missing || config_invalid;
}

bool ShouldAssignGeneratedEventId(bool event_id_empty)
{
    return event_id_empty;
}

bool ShouldRejectSerializedPayload(bool serialized_empty)
{
    return serialized_empty;
}

bool ShouldSetParseError(bool parsed, bool error_sink_present)
{
    return !parsed && error_sink_present;
}

bool ShouldCommitLastConsumed(bool consumer_present, bool last_message_present)
{
    return consumer_present && last_message_present;
}

bool ShouldNackLastConsumed(bool consumer_present, bool last_message_present)
{
    return consumer_present && last_message_present;
}

bool ShouldRouteToDlq(int retry_count, int consume_retry_max)
{
    return retry_count >= consume_retry_max;
}

int InitialOutboxStatus()
{
    return 0;
}

int InitialRetryCount()
{
    return 0;
}

const char* KafkaNotConfiguredError()
{
    return "kafka_not_configured";
}

const char* SerializeFailedError()
{
    return "serialize_failed";
}

const char* OutboxEnqueueFailedError()
{
    return "outbox_enqueue_failed";
}

const char* KafkaBuildDisabledError()
{
    return "kafka_build_disabled";
}

const char* ParseFailedError()
{
    return "parse_failed";
}

const char* ProducerUnavailableError()
{
    return "producer_unavailable";
}
} // namespace memochat::chat::messaging::kafka_event_bus::modules

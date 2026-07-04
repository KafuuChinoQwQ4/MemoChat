export module memochat.chat.redis_async_event_bus_algorithms;

export namespace memochat::chat::messaging::redis_event_bus::modules
{
bool ShouldRejectSerializedPayload(bool serialized_empty)
{
    return serialized_empty;
}

bool ShouldRequeueLastConsumed(bool last_topic_empty, bool last_serialized_empty)
{
    return !last_topic_empty && !last_serialized_empty;
}

const char* SerializeFailedError()
{
    return "serialize_failed";
}

const char* QueuePublishFailedError()
{
    return "queue_publish_failed";
}

const char* ParseFailedError()
{
    return "parse_failed";
}
} // namespace memochat::chat::messaging::redis_event_bus::modules

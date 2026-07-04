import memochat.chat.redis_async_event_bus_algorithms;

bool MemoChatTestRedisEventShouldRejectSerializedPayload(bool serialized_empty)
{
    return memochat::chat::messaging::redis_event_bus::modules::ShouldRejectSerializedPayload(serialized_empty);
}

bool MemoChatTestRedisEventShouldRequeueLastConsumed(bool last_topic_empty, bool last_serialized_empty)
{
    return memochat::chat::messaging::redis_event_bus::modules::ShouldRequeueLastConsumed(last_topic_empty,
                                                                                          last_serialized_empty);
}

const char* MemoChatTestRedisEventSerializeFailedError()
{
    return memochat::chat::messaging::redis_event_bus::modules::SerializeFailedError();
}

const char* MemoChatTestRedisEventQueuePublishFailedError()
{
    return memochat::chat::messaging::redis_event_bus::modules::QueuePublishFailedError();
}

const char* MemoChatTestRedisEventParseFailedError()
{
    return memochat::chat::messaging::redis_event_bus::modules::ParseFailedError();
}

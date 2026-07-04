import memochat.chat.async_event_dispatcher_algorithms;

bool MemoChatTestAsyncDispatchShouldRejectPublish(bool event_bus_present)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldRejectPublish(event_bus_present);
}

bool MemoChatTestAsyncDispatchShouldPauseWhenWorkerDisabled(bool worker_enabled)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldPauseWhenWorkerDisabled(worker_enabled);
}

bool MemoChatTestAsyncDispatchShouldPollEventBus(bool event_bus_present)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldPollEventBus(event_bus_present);
}

bool MemoChatTestAsyncDispatchShouldAckInvalidEvent(bool parsed)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldAckInvalidEvent(parsed);
}

bool MemoChatTestAsyncDispatchShouldSleepAfterPoll(bool handled)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldSleepAfterPoll(handled);
}

bool MemoChatTestAsyncDispatchShouldRejectJsonObject(bool is_object)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldRejectJsonObject(is_object);
}

bool MemoChatTestAsyncDispatchShouldInvalidateRelationBootstrapCache(int uid, bool cache_present)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldInvalidateRelationBootstrapCache(
        uid,
        cache_present);
}

bool MemoChatTestAsyncDispatchShouldAppendUniqueUid(int uid, bool already_present)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldAppendUniqueUid(uid, already_present);
}

bool MemoChatTestAsyncDispatchShouldNotifyMessageStatus(int uid)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldNotifyMessageStatus(uid);
}

bool MemoChatTestAsyncDispatchShouldRejectPrivateMessagePayload(int from_uid,
                                                                int to_uid,
                                                                bool text_array,
                                                                bool text_empty)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldRejectPrivateMessagePayload(from_uid,
                                                                                                         to_uid,
                                                                                                         text_array,
                                                                                                         text_empty);
}

bool MemoChatTestAsyncDispatchShouldKeepPrivateMessage(bool msg_id_empty, bool content_empty)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldKeepPrivateMessage(msg_id_empty,
                                                                                                content_empty);
}

bool MemoChatTestAsyncDispatchShouldUseCurrentTimestamp(long long created_at)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldUseCurrentTimestamp(created_at);
}

bool MemoChatTestAsyncDispatchShouldCaptureFirstMessageId(bool first_msg_id_empty)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldCaptureFirstMessageId(first_msg_id_empty);
}

bool MemoChatTestAsyncDispatchShouldPublishDialogSync(bool client_msg_id_empty)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldPublishDialogSync(client_msg_id_empty);
}

bool MemoChatTestAsyncDispatchShouldRejectGroupMessagePayload(int from_uid, long long group_id, bool msg_object)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldRejectGroupMessagePayload(from_uid,
                                                                                                       group_id,
                                                                                                       msg_object);
}

bool MemoChatTestAsyncDispatchShouldKeepGroupMessage(bool msg_id_empty, bool content_empty)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldKeepGroupMessage(msg_id_empty,
                                                                                              content_empty);
}

bool MemoChatTestAsyncDispatchShouldPushGroupDelivery(bool delivery_gateway_present)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldPushGroupDelivery(
        delivery_gateway_present);
}

bool MemoChatTestAsyncDispatchShouldRefreshDialogWarning(bool repository_present, bool refresh_ok)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldRefreshDialogWarning(repository_present,
                                                                                                  refresh_ok);
}

bool MemoChatTestAsyncDispatchShouldLogPrivateRouteWarning(bool stale_route, bool notify_delivered)
{
    return memochat::chat::messaging::async_event_dispatcher::modules::ShouldLogPrivateRouteWarning(stale_route,
                                                                                                    notify_delivered);
}

int MemoChatTestAsyncDispatchWorkerDisabledSleepMs()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::WorkerDisabledSleepMs();
}

int MemoChatTestAsyncDispatchIdlePollSleepMs()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::IdlePollSleepMs();
}

const char* MemoChatTestAsyncDispatchEventBusUnavailableError()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::EventBusUnavailableError();
}

const char* MemoChatTestAsyncDispatchInvalidEventLogEvent()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::InvalidEventLogEvent();
}

const char* MemoChatTestAsyncDispatchInvalidEventLogMessage()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::InvalidEventLogMessage();
}

const char* MemoChatTestAsyncDispatchUnknownTopicLogEvent()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::UnknownTopicLogEvent();
}

const char* MemoChatTestAsyncDispatchUnknownTopicLogMessage()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::UnknownTopicLogMessage();
}

const char* MemoChatTestAsyncDispatchPrivateRouteEvent()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::PrivateRouteEvent();
}

const char* MemoChatTestAsyncDispatchPrivateNotifyDeliveredMessage()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::PrivateNotifyDeliveredMessage();
}

const char* MemoChatTestAsyncDispatchPrivateNotifyNotDeliveredMessage()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::PrivateNotifyNotDeliveredMessage();
}

const char* MemoChatTestAsyncDispatchDialogSyncPublishFailedEvent()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::DialogSyncPublishFailedEvent();
}

const char* MemoChatTestAsyncDispatchPrivateDialogSyncPublishFailedMessage()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::PrivateDialogSyncPublishFailedMessage();
}

const char* MemoChatTestAsyncDispatchGroupDialogSyncPublishFailedMessage()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::GroupDialogSyncPublishFailedMessage();
}

const char* MemoChatTestAsyncDispatchDialogSyncRefreshFailedEvent()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::DialogSyncRefreshFailedEvent();
}

const char* MemoChatTestAsyncDispatchDialogSyncRefreshFailedMessage()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::DialogSyncRefreshFailedMessage();
}

const char* MemoChatTestAsyncDispatchRelationStateRefreshFailedEvent()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::RelationStateRefreshFailedEvent();
}

const char* MemoChatTestAsyncDispatchRelationStateRefreshFailedMessage()
{
    return memochat::chat::messaging::async_event_dispatcher::modules::RelationStateRefreshFailedMessage();
}

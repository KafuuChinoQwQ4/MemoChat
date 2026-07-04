export module memochat.chat.async_event_dispatcher_algorithms;

export namespace memochat::chat::messaging::async_event_dispatcher::modules
{
bool ShouldRejectPublish(bool event_bus_present)
{
    return !event_bus_present;
}

bool ShouldPauseWhenWorkerDisabled(bool worker_enabled)
{
    return !worker_enabled;
}

bool ShouldPollEventBus(bool event_bus_present)
{
    return event_bus_present;
}

bool ShouldAckInvalidEvent(bool parsed)
{
    return !parsed;
}

bool ShouldSleepAfterPoll(bool handled)
{
    return !handled;
}

bool ShouldRejectJsonObject(bool is_object)
{
    return !is_object;
}

bool ShouldInvalidateRelationBootstrapCache(int uid, bool cache_present)
{
    return uid > 0 && cache_present;
}

bool ShouldAppendUniqueUid(int uid, bool already_present)
{
    return uid > 0 && !already_present;
}

bool ShouldNotifyMessageStatus(int uid)
{
    return uid > 0;
}

bool ShouldRejectPrivateMessagePayload(int from_uid, int to_uid, bool text_array, bool text_empty)
{
    return from_uid <= 0 || to_uid <= 0 || !text_array || text_empty;
}

bool ShouldKeepPrivateMessage(bool msg_id_empty, bool content_empty)
{
    return !msg_id_empty && !content_empty;
}

bool ShouldUseCurrentTimestamp(long long created_at)
{
    return created_at <= 0;
}

bool ShouldCaptureFirstMessageId(bool first_msg_id_empty)
{
    return first_msg_id_empty;
}

bool ShouldPublishDialogSync(bool client_msg_id_empty)
{
    return !client_msg_id_empty;
}

bool ShouldRejectGroupMessagePayload(int from_uid, long long group_id, bool msg_object)
{
    return from_uid <= 0 || group_id <= 0 || !msg_object;
}

bool ShouldKeepGroupMessage(bool msg_id_empty, bool content_empty)
{
    return !msg_id_empty && !content_empty;
}

bool ShouldPushGroupDelivery(bool delivery_gateway_present)
{
    return delivery_gateway_present;
}

bool ShouldRefreshDialogWarning(bool repository_present, bool refresh_ok)
{
    return !repository_present || !refresh_ok;
}

bool ShouldLogPrivateRouteWarning(bool stale_route, bool notify_delivered)
{
    return stale_route || !notify_delivered;
}

int WorkerDisabledSleepMs()
{
    return 200;
}

int IdlePollSleepMs()
{
    return 50;
}

const char* EventBusUnavailableError()
{
    return "event_bus_unavailable";
}

const char* InvalidEventLogEvent()
{
    return "chat.async.invalid_event";
}

const char* InvalidEventLogMessage()
{
    return "async chat event parse failed";
}

const char* UnknownTopicLogEvent()
{
    return "chat.async.unknown_topic";
}

const char* UnknownTopicLogMessage()
{
    return "async chat event topic is not registered";
}

const char* PrivateRouteEvent()
{
    return "chat.private.route.async";
}

const char* PrivateNotifyDeliveredMessage()
{
    return "private message notify delivered";
}

const char* PrivateNotifyNotDeliveredMessage()
{
    return "private message notify not delivered";
}

const char* DialogSyncPublishFailedEvent()
{
    return "chat.dialog_sync.publish_failed";
}

const char* PrivateDialogSyncPublishFailedMessage()
{
    return "failed to publish private dialog sync event";
}

const char* GroupDialogSyncPublishFailedMessage()
{
    return "failed to publish group dialog sync event";
}

const char* DialogSyncRefreshFailedEvent()
{
    return "chat.dialog_sync.refresh_failed";
}

const char* DialogSyncRefreshFailedMessage()
{
    return "failed to refresh dialog runtime";
}

const char* RelationStateRefreshFailedEvent()
{
    return "chat.relation_state.refresh_failed";
}

const char* RelationStateRefreshFailedMessage()
{
    return "failed to refresh dialogs after relation event";
}
} // namespace memochat::chat::messaging::async_event_dispatcher::modules

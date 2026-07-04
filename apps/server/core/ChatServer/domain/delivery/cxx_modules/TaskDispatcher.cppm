export module memochat.chat.task_dispatcher_algorithms;

namespace memochat::chat::delivery::task_dispatcher::detail
{
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
} // namespace memochat::chat::delivery::task_dispatcher::detail

export namespace memochat::chat::delivery::task_dispatcher::modules
{
const char* MessageDeliveryRetryTaskType()
{
    return "message_delivery_retry";
}

const char* OfflineNotifyTaskType()
{
    return "offline_notify";
}

const char* RelationNotifyTaskType()
{
    return "relation_notify";
}

const char* OutboxRepairTaskType()
{
    return "outbox_repair";
}

const char* TaskBusUnavailableError()
{
    return "task_bus_unavailable";
}

const char* TaskHandlerFailedError()
{
    return "task_handler_failed";
}

const char* UnknownTaskTypeEventName()
{
    return "chat.task.unknown_type";
}

const char* UnknownTaskTypeMessage()
{
    return "task type is not registered";
}

int NoTaskSleepMs()
{
    return 50;
}

bool ShouldPublishTask(bool has_task_bus)
{
    return has_task_bus;
}

bool IsDeliveryTaskType(const char* data, unsigned long long size)
{
    return detail::EqualsLiteral(data, size, MessageDeliveryRetryTaskType(), 22) ||
           detail::EqualsLiteral(data, size, OfflineNotifyTaskType(), 14) ||
           detail::EqualsLiteral(data, size, RelationNotifyTaskType(), 15);
}

bool IsOutboxRepairTaskType(const char* data, unsigned long long size)
{
    return detail::EqualsLiteral(data, size, OutboxRepairTaskType(), 13);
}

bool ShouldAckInvalidDeliveryTaskPayload()
{
    return true;
}

bool ShouldExpediteOutboxRepair(bool has_scheduler, long long outbox_id)
{
    return has_scheduler && outbox_id > 0;
}
} // namespace memochat::chat::delivery::task_dispatcher::modules

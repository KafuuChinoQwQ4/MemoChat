import memochat.chat.task_dispatcher_algorithms;

#include <string>

bool MemoChatTestTaskDispatcherShouldPublish(bool has_task_bus)
{
    return memochat::chat::delivery::task_dispatcher::modules::ShouldPublishTask(has_task_bus);
}

bool MemoChatTestTaskDispatcherIsDeliveryTaskType(const std::string& task_type)
{
    return memochat::chat::delivery::task_dispatcher::modules::IsDeliveryTaskType(task_type.data(), task_type.size());
}

bool MemoChatTestTaskDispatcherIsOutboxRepairTaskType(const std::string& task_type)
{
    return memochat::chat::delivery::task_dispatcher::modules::IsOutboxRepairTaskType(task_type.data(),
                                                                                      task_type.size());
}

bool MemoChatTestTaskDispatcherShouldExpediteOutboxRepair(bool has_scheduler, long long outbox_id)
{
    return memochat::chat::delivery::task_dispatcher::modules::ShouldExpediteOutboxRepair(has_scheduler, outbox_id);
}

bool MemoChatTestTaskDispatcherShouldAckInvalidPayload()
{
    return memochat::chat::delivery::task_dispatcher::modules::ShouldAckInvalidDeliveryTaskPayload();
}

const char* MemoChatTestTaskDispatcherTaskBusUnavailableError()
{
    return memochat::chat::delivery::task_dispatcher::modules::TaskBusUnavailableError();
}

const char* MemoChatTestTaskDispatcherTaskHandlerFailedError()
{
    return memochat::chat::delivery::task_dispatcher::modules::TaskHandlerFailedError();
}

const char* MemoChatTestTaskDispatcherUnknownTaskTypeEventName()
{
    return memochat::chat::delivery::task_dispatcher::modules::UnknownTaskTypeEventName();
}

const char* MemoChatTestTaskDispatcherUnknownTaskTypeMessage()
{
    return memochat::chat::delivery::task_dispatcher::modules::UnknownTaskTypeMessage();
}

int MemoChatTestTaskDispatcherNoTaskSleepMs()
{
    return memochat::chat::delivery::task_dispatcher::modules::NoTaskSleepMs();
}

import memochat.chat.outbox_algorithms;

int MemoChatTestOutboxNextRetryCount(int retry_count)
{
    return memochat::chat::persistence::outbox::modules::NextRetryCount(retry_count);
}

int MemoChatTestOutboxSelectBackoffMs(int retry_base_ms, int retry_max_ms, int retry_count)
{
    return memochat::chat::persistence::outbox::modules::SelectBackoffMs(retry_base_ms, retry_max_ms, retry_count);
}

bool MemoChatTestOutboxTerminalRetry(int retry_count, int retry_max)
{
    return memochat::chat::persistence::outbox::modules::IsTerminalRetry(retry_count, retry_max);
}

bool MemoChatTestOutboxShouldScheduleRepairTask(bool terminal_retry, bool has_repair_task_publisher)
{
    return memochat::chat::persistence::outbox::modules::ShouldScheduleRepairTask(terminal_retry,
                                                                                  has_repair_task_publisher);
}

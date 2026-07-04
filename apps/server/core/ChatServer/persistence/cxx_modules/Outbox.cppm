export module memochat.chat.outbox_algorithms;

export namespace memochat::chat::persistence::outbox::modules
{
int AtLeast(int value, int minimum)
{
    return value < minimum ? minimum : value;
}

int MinInt(int left, int right)
{
    return left < right ? left : right;
}

int NextRetryCount(int retry_count)
{
    return retry_count + 1;
}

int SelectBackoffMs(int retry_base_ms, int retry_max_ms, int retry_count)
{
    return MinInt(retry_max_ms, retry_base_ms * AtLeast(retry_count, 1));
}

bool IsTerminalRetry(int retry_count, int retry_max)
{
    return retry_count >= retry_max;
}

bool ShouldScheduleRepairTask(bool terminal_retry, bool has_repair_task_publisher)
{
    return !terminal_retry && has_repair_task_publisher;
}
} // namespace memochat::chat::persistence::outbox::modules

import memochat.chat.inline_task_bus_algorithms;

bool MemoChatTestInlineTaskAcceptsAllRoutingKeys(bool routing_keys_empty)
{
    return memochat::chat::messaging::inline_task_bus::modules::AcceptsAllRoutingKeys(routing_keys_empty);
}

bool MemoChatTestInlineTaskShouldConsume(long long available_at_ms, long long now_ms)
{
    return memochat::chat::messaging::inline_task_bus::modules::ShouldConsumeAvailableTask(available_at_ms, now_ms);
}

bool MemoChatTestInlineTaskShouldDropAfterRetry(int retry_count, int max_retries)
{
    return memochat::chat::messaging::inline_task_bus::modules::ShouldDropAfterRetry(retry_count, max_retries);
}

int MemoChatTestInlineTaskRetryDelayMs()
{
    return memochat::chat::messaging::inline_task_bus::modules::RetryDelayMs();
}

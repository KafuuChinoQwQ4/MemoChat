export module memochat.chat.inline_task_bus_algorithms;

export namespace memochat::chat::messaging::inline_task_bus::modules
{
bool AcceptsAllRoutingKeys(bool routing_keys_empty)
{
    return routing_keys_empty;
}

bool ShouldConsumeAvailableTask(long long available_at_ms, long long now_ms)
{
    return available_at_ms <= now_ms;
}

bool ShouldDropAfterRetry(int retry_count, int max_retries)
{
    return retry_count > max_retries;
}

int RetryDelayMs()
{
    return 1000;
}
} // namespace memochat::chat::messaging::inline_task_bus::modules

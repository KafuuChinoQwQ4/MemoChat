export module memochat.runtime.io_context_pool_algorithms;

export namespace memochat::runtime::modules
{
unsigned long long NormalizeIoContextPoolSize(unsigned long long requested_size)
{
    return requested_size == 0 ? 1 : requested_size;
}

bool ShouldWrapNextIoContextIndex(unsigned long long next_index, unsigned long long pool_size)
{
    return pool_size != 0 && next_index >= pool_size;
}

bool ShouldStopIoContextPool(bool already_stopped)
{
    return !already_stopped;
}

bool ShouldJoinIoContextThread(bool joinable)
{
    return joinable;
}
} // namespace memochat::runtime::modules

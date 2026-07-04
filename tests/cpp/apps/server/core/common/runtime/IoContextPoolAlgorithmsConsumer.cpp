import memochat.runtime.io_context_pool_algorithms;

namespace memochat::tests::runtime
{
unsigned long long NormalizeIoContextPoolSize(unsigned long long requested_size)
{
    return memochat::runtime::modules::NormalizeIoContextPoolSize(requested_size);
}

bool ShouldWrapNextIoContextIndex(unsigned long long next_index, unsigned long long pool_size)
{
    return memochat::runtime::modules::ShouldWrapNextIoContextIndex(next_index, pool_size);
}

bool ShouldStopIoContextPool(bool already_stopped)
{
    return memochat::runtime::modules::ShouldStopIoContextPool(already_stopped);
}

bool ShouldJoinIoContextThread(bool joinable)
{
    return memochat::runtime::modules::ShouldJoinIoContextThread(joinable);
}
} // namespace memochat::tests::runtime

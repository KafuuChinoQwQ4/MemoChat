export module memochat.gate.worker_pool_algorithms;

export namespace memochat::gate::worker_pool::modules
{
unsigned long long SelectWorkerThreadCount(bool requested_zero,
                                           unsigned int hardware_concurrency,
                                           unsigned long long requested_threads)
{
    return requested_zero ? static_cast<unsigned long long>(hardware_concurrency) : requested_threads;
}

bool ShouldJoinWorkerPool(bool stopped)
{
    return !stopped;
}

bool ShouldStopWorkerPool(bool stopped)
{
    return !stopped;
}
} // namespace memochat::gate::worker_pool::modules

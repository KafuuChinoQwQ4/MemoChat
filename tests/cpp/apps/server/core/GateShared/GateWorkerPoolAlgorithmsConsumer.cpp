import memochat.gate.worker_pool_algorithms;

namespace memochat::tests::gate::worker_pool
{
unsigned long long
SelectWorkerThreadCount(bool requested_zero, unsigned int hardware_concurrency, unsigned long long requested_threads)
{
    return memochat::gate::worker_pool::modules::SelectWorkerThreadCount(requested_zero,
                                                                         hardware_concurrency,
                                                                         requested_threads);
}

bool ShouldJoinWorkerPool(bool stopped)
{
    return memochat::gate::worker_pool::modules::ShouldJoinWorkerPool(stopped);
}

bool ShouldStopWorkerPool(bool stopped)
{
    return memochat::gate::worker_pool::modules::ShouldStopWorkerPool(stopped);
}
} // namespace memochat::tests::gate::worker_pool

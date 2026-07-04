#include "GateWorkerPool.hpp"

import memochat.gate.worker_pool_algorithms;

namespace worker_pool_modules = memochat::gate::worker_pool::modules;

GateWorkerPool::GateWorkerPool(std::size_t num_threads)
    : _pool(static_cast<std::size_t>(
          worker_pool_modules::SelectWorkerThreadCount(num_threads == 0,
                                                       std::thread::hardware_concurrency(),
                                                       static_cast<unsigned long long>(num_threads))))
{
}

GateWorkerPool::~GateWorkerPool()
{
    try
    {
        if (worker_pool_modules::ShouldJoinWorkerPool(_stopped))
        {
            _pool.join();
        }
    }
    catch (...)
    {
    }
}

void GateWorkerPool::Stop()
{
    if (!worker_pool_modules::ShouldStopWorkerPool(_stopped))
    {
        return;
    }
    _stopped = true;
    _pool.join();
}

#include "GateWorkerPool.h"

GateWorkerPool::GateWorkerPool(std::size_t num_threads)
    : _pool(num_threads == 0 ? std::thread::hardware_concurrency() : num_threads) {
}

GateWorkerPool::~GateWorkerPool() {
    try {
        if (!_stopped) {
            _pool.join();
        }
    } catch (...) {}
}

void GateWorkerPool::Stop() {
    if (_stopped) return;
    _stopped = true;
    _pool.join();
}

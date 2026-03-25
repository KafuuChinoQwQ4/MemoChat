#include "H1WorkerPool.h"

H1WorkerPool::H1WorkerPool(std::size_t num_threads)
    : _pool(num_threads == 0 ? std::thread::hardware_concurrency() : num_threads) {
}

H1WorkerPool::~H1WorkerPool() {
    try {
        if (!_stopped) {
            _pool.join();
        }
    } catch (...) {}
}

void H1WorkerPool::Stop() {
    if (_stopped) return;
    _stopped = true;
    _pool.join();
}

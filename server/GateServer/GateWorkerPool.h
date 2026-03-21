#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include "Singleton.h"

class GateWorkerPool : public Singleton<GateWorkerPool> {
    friend class Singleton<GateWorkerPool>;
public:
    explicit GateWorkerPool(std::size_t num_threads = 0);
    ~GateWorkerPool();

    GateWorkerPool(const GateWorkerPool&) = delete;
    GateWorkerPool& operator=(const GateWorkerPool&) = delete;

    boost::asio::thread_pool& pool() { return _pool; }

    template<typename F>
    void post(F&& handler) {
        boost::asio::post(_pool, std::forward<F>(handler));
    }

    void Stop();

private:
    boost::asio::thread_pool _pool;
    bool _stopped = false;
};

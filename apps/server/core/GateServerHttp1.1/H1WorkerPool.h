#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include "Singleton.h"

class H1WorkerPool : public Singleton<H1WorkerPool> {
    friend class Singleton<H1WorkerPool>;
public:
    explicit H1WorkerPool(std::size_t num_threads = 0);
    ~H1WorkerPool();

    H1WorkerPool(const H1WorkerPool&) = delete;
    H1WorkerPool& operator=(const H1WorkerPool&) = delete;

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

#pragma once
#include <cstddef>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "Singleton.hpp"
#include "runtime/ExplicitThread.hpp"

class GateWorkerPool : public Singleton<GateWorkerPool>
{
    friend class Singleton<GateWorkerPool>;

public:
    GateWorkerPool() = default;
    ~GateWorkerPool() noexcept;

    GateWorkerPool(const GateWorkerPool&) = delete;
    GateWorkerPool& operator=(const GateWorkerPool&) = delete;

    bool Start(std::size_t num_threads, std::string* error = nullptr);

    template <typename F> bool post(F&& handler)
    {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_started || _stopping)
            {
                return false;
            }
            _tasks.emplace(std::forward<F>(handler));
        }
        _condition.notify_one();
        return true;
    }

    bool Stop(std::string* error = nullptr);

private:
    void RunWorker();

    std::mutex _mutex;
    std::condition_variable _condition;
    std::queue<std::function<void()>> _tasks;
    std::vector<memochat::runtime::ExplicitThread> _threads;
    bool _started = false;
    bool _stopping = false;
    bool _stopped = false;
};

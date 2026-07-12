#include "GateWorkerPool.hpp"

#include <iostream>

import memochat.gate.worker_pool_algorithms;

namespace worker_pool_modules = memochat::gate::worker_pool::modules;

bool GateWorkerPool::Start(std::size_t num_threads, std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_started)
        {
            return true;
        }
        if (_stopped)
        {
            if (error != nullptr)
            {
                *error = "worker pool is already stopped";
            }
            return false;
        }
    }

    const std::size_t thread_count = static_cast<std::size_t>(
        worker_pool_modules::SelectWorkerThreadCount(num_threads == 0,
                                                     memochat::runtime::ExplicitThread::HardwareConcurrency(),
                                                     static_cast<unsigned long long>(num_threads)));
    _threads.reserve(thread_count);
    for (std::size_t index = 0; index < thread_count; ++index)
    {
        _threads.emplace_back();
        std::string thread_error;
        if (_threads.back().Start(
                [this]
                {
                    RunWorker();
                },
                &thread_error))
        {
            continue;
        }

        _threads.pop_back();
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _stopping = true;
        }
        _condition.notify_all();
        for (auto& thread : _threads)
        {
            std::string join_error;
            if (!thread.Join(&join_error) && thread_error.empty())
            {
                thread_error = std::move(join_error);
            }
        }
        _threads.clear();
        _stopped = true;
        if (error != nullptr)
        {
            *error = "worker thread " + std::to_string(index) + " failed to start: " + thread_error;
        }
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _started = true;
    }
    return true;
}

GateWorkerPool::~GateWorkerPool() noexcept
{
    if (worker_pool_modules::ShouldJoinWorkerPool(_stopped))
    {
        std::string error;
        if (!Stop(&error))
        {
            std::cerr << "gate worker pool shutdown failed: " << error << std::endl;
        }
    }
}

bool GateWorkerPool::Stop(std::string* error)
{
    if (error != nullptr)
    {
        error->clear();
    }
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!worker_pool_modules::ShouldStopWorkerPool(_stopped))
        {
            return true;
        }
        _stopped = true;
        _stopping = true;
    }
    _condition.notify_all();

    bool joined = true;
    std::string first_join_error;
    for (auto& thread : _threads)
    {
        std::string join_error;
        if (!thread.Join(&join_error))
        {
            joined = false;
            if (first_join_error.empty())
            {
                first_join_error = std::move(join_error);
            }
        }
    }
    _threads.clear();
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _started = false;
    }
    if (!joined)
    {
        if (error != nullptr)
        {
            *error = std::move(first_join_error);
        }
        else
        {
            std::cerr << "gate worker pool join failed: " << first_join_error << std::endl;
        }
    }
    return joined;
}

void GateWorkerPool::RunWorker()
{
    for (;;)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _condition.wait(lock,
                            [this]
                            {
                                return _stopping || !_tasks.empty();
                            });
            if (_stopping && _tasks.empty())
            {
                return;
            }
            task = std::move(_tasks.front());
            _tasks.pop();
        }
        task();
    }
}

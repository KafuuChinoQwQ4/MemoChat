#include "ChatDeliveryRuntime.hpp"

#include "logging/Logger.hpp"

import memochat.chat.delivery_runtime_algorithms;

namespace delivery_runtime_modules = memochat::chat::delivery::runtime::modules;

ChatDeliveryRuntime::ChatDeliveryRuntime(LoopFn event_loop, LoopFn task_loop)
    : _event_loop(std::move(event_loop))
    , _task_loop(std::move(task_loop))
{
}

ChatDeliveryRuntime::~ChatDeliveryRuntime()
{
    StopAndJoin();
}

bool ChatDeliveryRuntime::Start(std::string* error)
{
    bool expected = delivery_runtime_modules::InitialStartedExpected();
    if (!_started.compare_exchange_strong(expected, true))
    {
        if (error != nullptr)
        {
            error->clear();
        }
        return true;
    }
    _stop_requested.store(delivery_runtime_modules::StopRequestedWhenStarting(), std::memory_order_release);
    if (!_event_worker_thread.Start(
            [this]() noexcept
            {
                RunLoop(_event_loop);
            },
            error))
    {
        _started.store(false, std::memory_order_release);
        _stop_requested.store(true, std::memory_order_release);
        return false;
    }
    if (!_task_worker_thread.Start(
            [this]() noexcept
            {
                RunLoop(_task_loop);
            },
            error))
    {
        _stop_requested.store(true, std::memory_order_release);
        std::string join_error;
        if (!_event_worker_thread.Join(&join_error))
        {
            memolog::LogError("chat.delivery.thread_join_failed",
                              "chat delivery event thread join failed after partial startup",
                              {{"error", join_error}});
        }
        _started.store(false, std::memory_order_release);
        return false;
    }
    return true;
}

void ChatDeliveryRuntime::StopAndJoin()
{
    _stop_requested.store(delivery_runtime_modules::StopRequestedWhenStopping(), std::memory_order_release);
    std::string error;
    if (delivery_runtime_modules::ShouldJoinThread(_event_worker_thread.Joinable()))
    {
        if (!_event_worker_thread.Join(&error))
        {
            memolog::LogError("chat.delivery.thread_join_failed",
                              "chat delivery event thread join failed",
                              {{"error", error}});
        }
    }
    if (delivery_runtime_modules::ShouldJoinThread(_task_worker_thread.Joinable()))
    {
        if (!_task_worker_thread.Join(&error))
        {
            memolog::LogError("chat.delivery.thread_join_failed",
                              "chat delivery task thread join failed",
                              {{"error", error}});
        }
    }
    _started.store(delivery_runtime_modules::StartedAfterStopAndJoin(), std::memory_order_release);
}

bool ChatDeliveryRuntime::StopRequested() const
{
    return _stop_requested.load(std::memory_order_acquire);
}

void ChatDeliveryRuntime::RunLoop(const LoopFn& loop)
{
    if (delivery_runtime_modules::ShouldRunLoop(static_cast<bool>(loop)))
    {
        loop();
    }
}

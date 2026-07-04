#include "ChatDeliveryRuntime.hpp"

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

void ChatDeliveryRuntime::Start()
{
    bool expected = delivery_runtime_modules::InitialStartedExpected();
    if (!_started.compare_exchange_strong(expected, true))
    {
        return;
    }
    _stop_requested.store(delivery_runtime_modules::StopRequestedWhenStarting(), std::memory_order_release);
    _event_worker_thread = std::thread(&ChatDeliveryRuntime::RunLoop, this, std::cref(_event_loop));
    _task_worker_thread = std::thread(&ChatDeliveryRuntime::RunLoop, this, std::cref(_task_loop));
}

void ChatDeliveryRuntime::StopAndJoin()
{
    _stop_requested.store(delivery_runtime_modules::StopRequestedWhenStopping(), std::memory_order_release);
    if (delivery_runtime_modules::ShouldJoinThread(_event_worker_thread.joinable()))
    {
        _event_worker_thread.join();
    }
    if (delivery_runtime_modules::ShouldJoinThread(_task_worker_thread.joinable()))
    {
        _task_worker_thread.join();
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

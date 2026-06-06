#include "ChatDeliveryRuntime.h"

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
    bool expected = false;
    if (!_started.compare_exchange_strong(expected, true))
    {
        return;
    }
    _stop_requested.store(false, std::memory_order_release);
    _event_worker_thread = std::thread(&ChatDeliveryRuntime::RunLoop, this, std::cref(_event_loop));
    _task_worker_thread = std::thread(&ChatDeliveryRuntime::RunLoop, this, std::cref(_task_loop));
}

void ChatDeliveryRuntime::StopAndJoin()
{
    _stop_requested.store(true, std::memory_order_release);
    if (_event_worker_thread.joinable())
    {
        _event_worker_thread.join();
    }
    if (_task_worker_thread.joinable())
    {
        _task_worker_thread.join();
    }
    _started.store(false, std::memory_order_release);
}

bool ChatDeliveryRuntime::StopRequested() const
{
    return _stop_requested.load(std::memory_order_acquire);
}

void ChatDeliveryRuntime::RunLoop(const LoopFn& loop)
{
    if (loop)
    {
        loop();
    }
}

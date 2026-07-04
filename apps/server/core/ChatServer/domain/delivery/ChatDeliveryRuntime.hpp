#pragma once

#include <atomic>
#include <functional>
#include <thread>

class ChatDeliveryRuntime
{
public:
    using LoopFn = std::function<void()>;

    ChatDeliveryRuntime(LoopFn event_loop, LoopFn task_loop);
    ~ChatDeliveryRuntime();

    void Start();
    void StopAndJoin();
    bool StopRequested() const;

private:
    void RunLoop(const LoopFn& loop);

    LoopFn _event_loop;
    LoopFn _task_loop;
    std::thread _event_worker_thread;
    std::thread _task_worker_thread;
    std::atomic<bool> _stop_requested{false};
    std::atomic<bool> _started{false};
};

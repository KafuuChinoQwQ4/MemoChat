#pragma once

#include "runtime/ExplicitThread.hpp"

#include <atomic>
#include <functional>
#include <string>

class ChatDeliveryRuntime
{
public:
    using LoopFn = std::function<void()>;

    ChatDeliveryRuntime(LoopFn event_loop, LoopFn task_loop);
    ~ChatDeliveryRuntime();

    bool Start(std::string* error = nullptr);
    void StopAndJoin();
    bool StopRequested() const;

private:
    void RunLoop(const LoopFn& loop);

    LoopFn _event_loop;
    LoopFn _task_loop;
    memochat::runtime::ExplicitThread _event_worker_thread;
    memochat::runtime::ExplicitThread _task_worker_thread;
    std::atomic<bool> _stop_requested{false};
    std::atomic<bool> _started{false};
};

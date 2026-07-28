#include "PostgresHealthMonitor.hpp"

#include <utility>

namespace memochat::chat::persistence
{
namespace
{
void SetError(std::string* error, std::string message)
{
    if (error != nullptr)
    {
        *error = std::move(message);
    }
}
} // namespace

PostgresHealthMonitor::~PostgresHealthMonitor()
{
    Stop();
}

bool PostgresHealthMonitor::Start(std::string chat_connection_string,
                                  std::string account_connection_string,
                                  PostgresHealthProbe probe,
                                  std::chrono::milliseconds interval,
                                  bool initially_healthy,
                                  std::string* error) noexcept
{
    if (error != nullptr)
    {
        error->clear();
    }
    if (thread_.Joinable())
    {
        healthy_.store(false, std::memory_order_release);
        SetError(error, "PostgreSQL health monitor is already running");
        return false;
    }
    if (chat_connection_string.empty() || account_connection_string.empty() || probe.function == nullptr ||
        interval <= std::chrono::milliseconds::zero())
    {
        healthy_.store(false, std::memory_order_release);
        SetError(error, "PostgreSQL health monitor configuration is invalid");
        return false;
    }

    chat_connection_string_ = std::move(chat_connection_string);
    account_connection_string_ = std::move(account_connection_string);
    probe_ = probe;
    interval_ = interval;
    stop_.store(false, std::memory_order_release);
    healthy_.store(initially_healthy, std::memory_order_release);

    std::string thread_error;
    if (!thread_.Start(
            [this]() noexcept
            {
                Run();
            },
            &thread_error))
    {
        stop_.store(true, std::memory_order_release);
        healthy_.store(false, std::memory_order_release);
        SetError(error, "PostgreSQL health monitor thread start failed: " + thread_error);
        return false;
    }
    return true;
}

bool PostgresHealthMonitor::Stop(std::string* error) noexcept
{
    if (error != nullptr)
    {
        error->clear();
    }
    stop_.store(true, std::memory_order_release);
    healthy_.store(false, std::memory_order_release);
    wait_condition_.notify_all();
    if (!thread_.Joinable())
    {
        return true;
    }
    const bool joined = thread_.Join(error);
    healthy_.store(false, std::memory_order_release);
    return joined;
}

bool PostgresHealthMonitor::Healthy() const noexcept
{
    return healthy_.load(std::memory_order_acquire);
}

void PostgresHealthMonitor::Run() noexcept
{
    while (!stop_.load(std::memory_order_acquire))
    {
        ProbeOnce();

        std::unique_lock<std::mutex> lock(wait_mutex_);
        if (wait_condition_.wait_for(lock,
                                     interval_,
                                     [this]
                                     {
                                         return stop_.load(std::memory_order_acquire);
                                     }))
        {
            break;
        }
    }
}

void PostgresHealthMonitor::ProbeOnce() noexcept
{
    const bool chat_healthy = probe_.Check(chat_connection_string_);
    if (stop_.load(std::memory_order_acquire))
    {
        healthy_.store(false, std::memory_order_release);
        return;
    }
    const bool account_healthy = probe_.Check(account_connection_string_);
    healthy_.store(chat_healthy && account_healthy && !stop_.load(std::memory_order_acquire),
                   std::memory_order_release);
}

} // namespace memochat::chat::persistence

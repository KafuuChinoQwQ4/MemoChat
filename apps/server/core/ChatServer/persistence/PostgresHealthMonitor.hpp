#pragma once

#include "runtime/ExplicitThread.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <string_view>

namespace memochat::chat::persistence
{

struct PostgresHealthProbe
{
    using Function = bool (*)(void* context, std::string_view connection_string) noexcept;

    void* context = nullptr;
    Function function = nullptr;

    [[nodiscard]] bool Check(std::string_view connection_string) const noexcept
    {
        return function != nullptr && function(context, connection_string);
    }
};

class PostgresHealthMonitor final
{
public:
    PostgresHealthMonitor() noexcept = default;
    ~PostgresHealthMonitor();

    PostgresHealthMonitor(const PostgresHealthMonitor&) = delete;
    PostgresHealthMonitor& operator=(const PostgresHealthMonitor&) = delete;

    bool Start(std::string chat_connection_string,
               std::string account_connection_string,
               PostgresHealthProbe probe,
               std::chrono::milliseconds interval,
               bool initially_healthy,
               std::string* error = nullptr) noexcept;
    bool Stop(std::string* error = nullptr) noexcept;

    [[nodiscard]] bool Healthy() const noexcept;

private:
    void Run() noexcept;
    void ProbeOnce() noexcept;

    std::string chat_connection_string_;
    std::string account_connection_string_;
    PostgresHealthProbe probe_;
    std::chrono::milliseconds interval_{0};
    std::atomic<bool> healthy_{false};
    std::atomic<bool> stop_{true};
    std::mutex wait_mutex_;
    std::condition_variable wait_condition_;
    memochat::runtime::ExplicitThread thread_;
};

} // namespace memochat::chat::persistence

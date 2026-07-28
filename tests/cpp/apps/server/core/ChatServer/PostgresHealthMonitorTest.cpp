#include "PostgresHealthMonitor.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <string>
#include <string_view>
#include <thread>

namespace
{
using memochat::chat::persistence::PostgresHealthMonitor;
using memochat::chat::persistence::PostgresHealthProbe;

struct FakeProbeState
{
    std::atomic<bool> chat_healthy{true};
    std::atomic<bool> account_healthy{true};
    std::atomic<int> chat_checks{0};
    std::atomic<int> account_checks{0};
    std::atomic<bool> block_chat{false};
    std::atomic<bool> chat_entered{false};
    std::atomic<bool> release_chat{false};
};

bool FakeProbe(void* context, std::string_view connection_string) noexcept
{
    auto& state = *static_cast<FakeProbeState*>(context);
    if (connection_string == "chat")
    {
        state.chat_checks.fetch_add(1, std::memory_order_relaxed);
        if (state.block_chat.load(std::memory_order_acquire))
        {
            state.chat_entered.store(true, std::memory_order_release);
            while (!state.release_chat.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }
        }
        return state.chat_healthy.load(std::memory_order_acquire);
    }
    if (connection_string == "account")
    {
        state.account_checks.fetch_add(1, std::memory_order_relaxed);
        return state.account_healthy.load(std::memory_order_acquire);
    }
    return false;
}

template <typename Predicate>
bool WaitUntil(Predicate&& predicate, std::chrono::milliseconds timeout = std::chrono::seconds(1))
{
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline)
    {
        if (predicate())
        {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return predicate();
}

TEST(PostgresHealthMonitorTest, PublishesFailureAndRecoveryForBothDatabases)
{
    FakeProbeState state;
    PostgresHealthMonitor monitor;
    std::string error;

    ASSERT_TRUE(monitor.Start("chat",
                              "account",
                              PostgresHealthProbe{.context = &state, .function = &FakeProbe},
                              std::chrono::milliseconds(5),
                              true,
                              &error))
        << error;
    ASSERT_TRUE(WaitUntil(
        [&state]
        {
            return state.chat_checks.load(std::memory_order_relaxed) > 0 &&
                   state.account_checks.load(std::memory_order_relaxed) > 0;
        }));
    EXPECT_TRUE(monitor.Healthy());

    state.account_healthy.store(false, std::memory_order_release);
    ASSERT_TRUE(WaitUntil(
        [&monitor]
        {
            return !monitor.Healthy();
        }));

    state.account_healthy.store(true, std::memory_order_release);
    ASSERT_TRUE(WaitUntil(
        [&monitor]
        {
            return monitor.Healthy();
        }));

    const int account_checks_before_chat_failure = state.account_checks.load(std::memory_order_relaxed);
    state.chat_healthy.store(false, std::memory_order_release);
    ASSERT_TRUE(WaitUntil(
        [&monitor]
        {
            return !monitor.Healthy();
        }));
    EXPECT_GT(state.account_checks.load(std::memory_order_relaxed), account_checks_before_chat_failure)
        << "the account database must still be probed when the chat database is down";

    ASSERT_TRUE(monitor.Stop(&error)) << error;
    EXPECT_FALSE(monitor.Healthy());
}

TEST(PostgresHealthMonitorTest, RejectsInvalidProbeAndFailsClosed)
{
    PostgresHealthMonitor monitor;
    std::string error;

    EXPECT_FALSE(monitor.Start("chat", "account", PostgresHealthProbe{}, std::chrono::milliseconds(5), true, &error));
    EXPECT_FALSE(monitor.Healthy());
    EXPECT_FALSE(error.empty());
}

TEST(PostgresHealthMonitorTest, StopRemainsFailClosedAfterAnInFlightSuccessfulProbe)
{
    FakeProbeState state;
    state.block_chat.store(true, std::memory_order_release);
    PostgresHealthMonitor monitor;
    std::string error;

    ASSERT_TRUE(monitor.Start("chat",
                              "account",
                              PostgresHealthProbe{.context = &state, .function = &FakeProbe},
                              std::chrono::hours(1),
                              true,
                              &error))
        << error;
    ASSERT_TRUE(WaitUntil(
        [&state]
        {
            return state.chat_entered.load(std::memory_order_acquire);
        }));

    memochat::runtime::ExplicitThread release_thread;
    std::string release_thread_error;
    ASSERT_TRUE(release_thread.Start(
        [&monitor, &state]() noexcept
        {
            while (monitor.Healthy())
            {
                std::this_thread::yield();
            }
            state.release_chat.store(true, std::memory_order_release);
        },
        &release_thread_error))
        << release_thread_error;
    ASSERT_TRUE(monitor.Stop(&error)) << error;
    ASSERT_TRUE(release_thread.Join(&release_thread_error)) << release_thread_error;
    EXPECT_FALSE(monitor.Healthy());
}

TEST(PostgresHealthMonitorTest, DestructorInterruptsTheProbeIntervalAndJoins)
{
    FakeProbeState state;
    auto destruction_start = std::chrono::steady_clock::time_point{};
    {
        PostgresHealthMonitor monitor;
        std::string error;
        ASSERT_TRUE(monitor.Start("chat",
                                  "account",
                                  PostgresHealthProbe{.context = &state, .function = &FakeProbe},
                                  std::chrono::hours(1),
                                  true,
                                  &error))
            << error;
        ASSERT_TRUE(WaitUntil(
            [&state]
            {
                return state.account_checks.load(std::memory_order_relaxed) > 0;
            }));
        destruction_start = std::chrono::steady_clock::now();
    }

    EXPECT_LT(std::chrono::steady_clock::now() - destruction_start, std::chrono::milliseconds(250));
}
} // namespace

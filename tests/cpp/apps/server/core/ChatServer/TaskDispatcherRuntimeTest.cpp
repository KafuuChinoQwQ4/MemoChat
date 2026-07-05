#include "TaskDispatcher.hpp"

#include "IAsyncTaskBus.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
#include <stdexcept>

namespace
{
class ThrowingTaskBus final : public IAsyncTaskBus
{
public:
    explicit ThrowingTaskBus(std::atomic_bool& stop_requested)
        : stop_requested_(stop_requested)
    {
    }

    bool Publish(const TaskEnvelope&, std::string*) override
    {
        return false;
    }

    bool ConsumeOnce(const std::vector<std::string>&, ConsumedTask&, std::string*) override
    {
        ++consume_calls;
        stop_requested_.store(true, std::memory_order_release);
        throw std::bad_alloc();
    }

    void AckLastConsumed() override
    {
        ++ack_calls;
    }

    void NackLastConsumed(const std::string&) override
    {
        ++nack_calls;
    }

    int consume_calls = 0;
    int ack_calls = 0;
    int nack_calls = 0;

private:
    std::atomic_bool& stop_requested_;
};
} // namespace

TEST(TaskDispatcherRuntimeTest, ConsumeExceptionDoesNotEscapeWorkerLoop)
{
    std::atomic_bool stop_requested{false};
    auto task_bus = std::make_shared<ThrowingTaskBus>(stop_requested);
    auto* task_bus_raw = task_bus.get();
    TaskDispatcher dispatcher(
        std::move(task_bus),
        [&stop_requested]()
        {
            return stop_requested.load(std::memory_order_acquire);
        },
        nullptr,
        nullptr);

    EXPECT_NO_THROW(dispatcher.DealTasks());
    EXPECT_EQ(task_bus_raw->consume_calls, 1);
    EXPECT_EQ(task_bus_raw->ack_calls, 0);
    EXPECT_EQ(task_bus_raw->nack_calls, 0);
}

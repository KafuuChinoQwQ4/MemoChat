#include "TaskDispatcher.hpp"

#include "IAsyncTaskBus.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <memory>
namespace
{
class FailingTaskBus final : public IAsyncTaskBus
{
public:
    explicit FailingTaskBus(std::atomic_bool& stop_requested)
        : stop_requested_(stop_requested)
    {
    }

    bool Publish(const TaskEnvelope&, std::string*) override
    {
        return false;
    }

    bool ConsumeOnce(const std::vector<std::string>&, ConsumedTask&, std::string* error) override
    {
        ++consume_calls;
        stop_requested_.store(true, std::memory_order_release);
        if (error != nullptr)
        {
            *error = "consume_failed";
        }
        return false;
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

TEST(TaskDispatcherRuntimeTest, ConsumeFailureStopsCleanlyWhenRequested)
{
    std::atomic_bool stop_requested{false};
    auto task_bus = std::make_shared<FailingTaskBus>(stop_requested);
    auto* task_bus_raw = task_bus.get();
    TaskDispatcher dispatcher(
        std::move(task_bus),
        [&stop_requested]()
        {
            return stop_requested.load(std::memory_order_acquire);
        },
        nullptr,
        nullptr);

    dispatcher.DealTasks();
    EXPECT_EQ(task_bus_raw->consume_calls, 1);
    EXPECT_EQ(task_bus_raw->ack_calls, 0);
    EXPECT_EQ(task_bus_raw->nack_calls, 0);
}

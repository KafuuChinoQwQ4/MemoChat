#include <gtest/gtest.h>

#include "runtime/ExplicitThread.hpp"

#include <atomic>
#include <string>

namespace
{

TEST(ExplicitThread, StartsAndJoinsWithReturnStatus)
{
    std::atomic<bool> ran = false;
    memochat::runtime::ExplicitThread thread;
    std::string error;

    ASSERT_TRUE(thread.Start(
        [&ran]() noexcept
        {
            ran.store(true);
        },
        &error))
        << error;
    EXPECT_TRUE(thread.Joinable());
    ASSERT_TRUE(thread.Join(&error)) << error;
    EXPECT_TRUE(ran.load());
    EXPECT_FALSE(thread.Joinable());
}

TEST(ExplicitThread, RejectsASecondStart)
{
    std::atomic<bool> release = false;
    memochat::runtime::ExplicitThread thread;
    std::string error;

    ASSERT_TRUE(thread.Start(
        [&release]() noexcept
        {
            while (!release.load())
            {
            }
        },
        &error));
    EXPECT_FALSE(thread.Start([]() noexcept {}, &error));
    EXPECT_EQ(error, "thread is already running");

    release.store(true);
    ASSERT_TRUE(thread.Join(&error)) << error;
}

TEST(ExplicitThread, ReportsHardwareConcurrency)
{
    EXPECT_GT(memochat::runtime::ExplicitThread::HardwareConcurrency(), 0U);
}

} // namespace

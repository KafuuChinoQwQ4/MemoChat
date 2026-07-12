#include "AuthLocalFallbackCounterStore.hpp"
#include "runtime/ExplicitThread.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <string>
#include <vector>

namespace
{
using Store = gateauthsupport::AuthLocalFallbackCounterStore;
using Status = gateauthsupport::AuthLocalFallbackCounterStatus;
using Clock = Store::Clock;
} // namespace

TEST(AuthLocalFallbackCounterStoreTest, FailsClosedForNewKeyAtCapacityButIncrementsExistingKey)
{
    Store store(2);
    const Clock::time_point now{};

    EXPECT_EQ(store.Increment("a", 10, now).status, Status::Incremented);
    EXPECT_EQ(store.Increment("b", 10, now).status, Status::Incremented);
    EXPECT_EQ(store.Increment("c", 10, now).status, Status::CapacityExhausted);

    const auto existing = store.Increment("a", 10, now + std::chrono::seconds(1));
    EXPECT_EQ(existing.status, Status::Incremented);
    EXPECT_EQ(existing.count, 2);
}

TEST(AuthLocalFallbackCounterStoreTest, PrunesExpiredEntriesBeforeApplyingCapacity)
{
    Store store(2);
    const Clock::time_point now{};

    ASSERT_EQ(store.Increment("a", 2, now).status, Status::Incremented);
    ASSERT_EQ(store.Increment("b", 2, now).status, Status::Incremented);

    const auto after_expiry = now + std::chrono::seconds(3);
    EXPECT_EQ(store.Increment("c", 2, after_expiry).status, Status::Incremented);
    EXPECT_EQ(store.Increment("d", 2, after_expiry).status, Status::Incremented);
    EXPECT_EQ(store.Increment("e", 2, after_expiry).status, Status::CapacityExhausted);
}

TEST(AuthLocalFallbackCounterStoreTest, ExpiredKeyRestartsAtOneWithFreshTtl)
{
    Store store(1);
    const Clock::time_point now{};

    const auto first = store.Increment("a", 5, now);
    ASSERT_EQ(first.status, Status::Incremented);
    EXPECT_EQ(first.count, 1);
    EXPECT_EQ(first.ttl_sec, 5);

    const auto second = store.Increment("a", 5, now + std::chrono::seconds(2));
    ASSERT_EQ(second.status, Status::Incremented);
    EXPECT_EQ(second.count, 2);
    EXPECT_EQ(second.ttl_sec, 3);

    const auto reset = store.Increment("a", 5, now + std::chrono::seconds(5));
    EXPECT_EQ(reset.status, Status::Incremented);
    EXPECT_EQ(reset.count, 1);
    EXPECT_EQ(reset.ttl_sec, 5);
}

TEST(AuthLocalFallbackCounterStoreTest, ConcurrentExistingKeyIncrementsAreExact)
{
    Store store(2);
    const Clock::time_point now{};
    constexpr int kThreads = 8;
    constexpr int kIncrementsPerThread = 250;
    std::vector<memochat::runtime::ExplicitThread> workers(kThreads);

    for (auto& worker : workers)
    {
        std::string error;
        ASSERT_TRUE(worker.Start(
            [&store, now]() noexcept
            {
                for (int index = 0; index < kIncrementsPerThread; ++index)
                {
                    EXPECT_EQ(store.Increment("shared", 30, now).status, Status::Incremented);
                }
            },
            &error))
            << error;
    }
    for (auto& worker : workers)
    {
        std::string error;
        ASSERT_TRUE(worker.Join(&error)) << error;
    }

    const auto final = store.Increment("shared", 30, now);
    EXPECT_EQ(final.status, Status::Incremented);
    EXPECT_EQ(final.count, kThreads * kIncrementsPerThread + 1);
}

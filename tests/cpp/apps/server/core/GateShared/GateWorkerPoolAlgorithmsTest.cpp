#include <gtest/gtest.h>

namespace memochat::tests::gate::worker_pool
{
unsigned long long
SelectWorkerThreadCount(bool requested_zero, unsigned int hardware_concurrency, unsigned long long requested_threads);
bool ShouldJoinWorkerPool(bool stopped);
bool ShouldStopWorkerPool(bool stopped);
} // namespace memochat::tests::gate::worker_pool

TEST(GateWorkerPoolAlgorithmsTest, SelectsHardwareConcurrencyOnlyWhenThreadCountIsNotRequested)
{
    EXPECT_EQ(memochat::tests::gate::worker_pool::SelectWorkerThreadCount(true, 8, 3), 8ULL);
    EXPECT_EQ(memochat::tests::gate::worker_pool::SelectWorkerThreadCount(true, 0, 3), 0ULL);
    EXPECT_EQ(memochat::tests::gate::worker_pool::SelectWorkerThreadCount(false, 8, 3), 3ULL);
}

TEST(GateWorkerPoolAlgorithmsTest, JoinsAndStopsOnlyOnce)
{
    EXPECT_TRUE(memochat::tests::gate::worker_pool::ShouldJoinWorkerPool(false));
    EXPECT_FALSE(memochat::tests::gate::worker_pool::ShouldJoinWorkerPool(true));

    EXPECT_TRUE(memochat::tests::gate::worker_pool::ShouldStopWorkerPool(false));
    EXPECT_FALSE(memochat::tests::gate::worker_pool::ShouldStopWorkerPool(true));
}

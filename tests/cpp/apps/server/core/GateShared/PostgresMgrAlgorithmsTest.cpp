#include <gtest/gtest.h>

namespace memochat::tests::gate::postgres_mgr
{
unsigned AccountForwardCount();
unsigned CallForwardCount();
unsigned MediaForwardCount();
unsigned MomentForwardCount();
unsigned ForwardingSurfaceCount();
bool IsCompleteForwardingSurface(unsigned account_count,
                                 unsigned call_count,
                                 unsigned media_count,
                                 unsigned moment_count);
} // namespace memochat::tests::gate::postgres_mgr

TEST(PostgresMgrAlgorithmsTest, PerDomainForwardCountsMatchFacade)
{
    EXPECT_EQ(memochat::tests::gate::postgres_mgr::AccountForwardCount(), 17u);
    EXPECT_EQ(memochat::tests::gate::postgres_mgr::CallForwardCount(), 2u);
    EXPECT_EQ(memochat::tests::gate::postgres_mgr::MediaForwardCount(), 2u);
    EXPECT_EQ(memochat::tests::gate::postgres_mgr::MomentForwardCount(), 17u);
}

TEST(PostgresMgrAlgorithmsTest, TotalForwardingSurfaceIsThirtyEight)
{
    EXPECT_EQ(memochat::tests::gate::postgres_mgr::ForwardingSurfaceCount(), 38u);
}

TEST(PostgresMgrAlgorithmsTest, CompleteSurfaceOnlyForExactCounts)
{
    EXPECT_TRUE(memochat::tests::gate::postgres_mgr::IsCompleteForwardingSurface(17u, 2u, 2u, 17u));
    EXPECT_FALSE(memochat::tests::gate::postgres_mgr::IsCompleteForwardingSurface(15u, 2u, 2u, 17u));
    EXPECT_FALSE(memochat::tests::gate::postgres_mgr::IsCompleteForwardingSurface(16u, 3u, 2u, 17u));
    EXPECT_FALSE(memochat::tests::gate::postgres_mgr::IsCompleteForwardingSurface(16u, 2u, 1u, 17u));
    EXPECT_FALSE(memochat::tests::gate::postgres_mgr::IsCompleteForwardingSurface(16u, 2u, 2u, 16u));
}

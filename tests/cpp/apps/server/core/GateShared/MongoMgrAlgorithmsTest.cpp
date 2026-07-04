#include <gtest/gtest.h>

namespace memochat::tests::gate::mongo_mgr
{
unsigned MomentContentForwardCount();
unsigned ForwardingSurfaceCount();
bool IsCompleteForwardingSurface(unsigned moment_content_count);
} // namespace memochat::tests::gate::mongo_mgr

TEST(MongoMgrAlgorithmsTest, MomentContentForwardCountIsThree)
{
    EXPECT_EQ(memochat::tests::gate::mongo_mgr::MomentContentForwardCount(), 3u);
}

TEST(MongoMgrAlgorithmsTest, TotalForwardingSurfaceIsThree)
{
    EXPECT_EQ(memochat::tests::gate::mongo_mgr::ForwardingSurfaceCount(), 3u);
}

TEST(MongoMgrAlgorithmsTest, CompleteSurfaceOnlyForExactCount)
{
    EXPECT_TRUE(memochat::tests::gate::mongo_mgr::IsCompleteForwardingSurface(3u));
    EXPECT_FALSE(memochat::tests::gate::mongo_mgr::IsCompleteForwardingSurface(2u));
    EXPECT_FALSE(memochat::tests::gate::mongo_mgr::IsCompleteForwardingSurface(4u));
}

#include <gtest/gtest.h>

unsigned MemoChatTestRelationAdapterQueryForwardCount();
unsigned MemoChatTestRelationAdapterCommandForwardCount();
unsigned MemoChatTestRelationAdapterTotalForwardCount();
long long MemoChatTestRelationAdapterDefaultTimeoutMs();
bool MemoChatTestRelationAdapterUsesDefaultTimeout(long long timeout_ms);
bool MemoChatTestRelationAdapterDidForwardExpectedSurface(unsigned query_count, unsigned command_count);

TEST(RelationGrpcServiceAdapterAlgorithmsTest, ExposesForwardedRelationSurfaceCounts)
{
    EXPECT_EQ(MemoChatTestRelationAdapterQueryForwardCount(), 3U);
    EXPECT_EQ(MemoChatTestRelationAdapterCommandForwardCount(), 8U);
    EXPECT_EQ(MemoChatTestRelationAdapterTotalForwardCount(), 11U);
}

TEST(RelationGrpcServiceAdapterAlgorithmsTest, KeepsDefaultTimeoutContract)
{
    EXPECT_EQ(MemoChatTestRelationAdapterDefaultTimeoutMs(), 2000LL);
    EXPECT_TRUE(MemoChatTestRelationAdapterUsesDefaultTimeout(2000LL));
    EXPECT_FALSE(MemoChatTestRelationAdapterUsesDefaultTimeout(1999LL));
    EXPECT_FALSE(MemoChatTestRelationAdapterUsesDefaultTimeout(0LL));
}

TEST(RelationGrpcServiceAdapterAlgorithmsTest, ClassifiesCompleteForwardingSurface)
{
    EXPECT_TRUE(MemoChatTestRelationAdapterDidForwardExpectedSurface(3U, 8U));
    EXPECT_FALSE(MemoChatTestRelationAdapterDidForwardExpectedSurface(2U, 8U));
    EXPECT_FALSE(MemoChatTestRelationAdapterDidForwardExpectedSurface(3U, 7U));
    EXPECT_FALSE(MemoChatTestRelationAdapterDidForwardExpectedSurface(10U, 0U));
}

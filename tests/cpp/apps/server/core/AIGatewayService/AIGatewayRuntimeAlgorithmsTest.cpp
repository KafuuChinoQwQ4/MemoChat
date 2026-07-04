#include <gtest/gtest.h>

#include "AIGatewayRuntime.hpp"

TEST(AIGatewayRuntimeAlgorithmsTest, SelectsConfiguredOrDefaultListenPort)
{
    EXPECT_EQ(SelectAIGatewayListenPort("", 8093), 8093);
    EXPECT_EQ(SelectAIGatewayListenPort("8094", 8093), 8094);
    EXPECT_EQ(SelectAIGatewayListenPort("70000", 8093), static_cast<unsigned short>(70000));
}

TEST(AIGatewayRuntimeAlgorithmsTest, NormalizesIoThreadCountLikeEntrypoint)
{
    EXPECT_EQ(NormalizeAIGatewayIoThreads(0), 4U);
    EXPECT_EQ(NormalizeAIGatewayIoThreads(1), 4U);
    EXPECT_EQ(NormalizeAIGatewayIoThreads(2), 2U);
    EXPECT_EQ(NormalizeAIGatewayIoThreads(12), 12U);
}

TEST(AIGatewayRuntimeAlgorithmsTest, SelectsWorkerThreadFallbackOnlyForZero)
{
    EXPECT_EQ(SelectAIGatewayWorkerThreads("", 8), 8U);
    EXPECT_EQ(SelectAIGatewayWorkerThreads("0", 8), 8U);
    EXPECT_EQ(SelectAIGatewayWorkerThreads("1", 8), 1U);
    EXPECT_EQ(SelectAIGatewayWorkerThreads("16", 8), 16U);
}

#include "app/GateDomainRuntime.hpp"

#include <gtest/gtest.h>

TEST(GateDomainRuntimeTest, SelectsConfiguredOrDefaultListenPort)
{
    EXPECT_EQ(SelectGateDomainListenPort("", 8102), 8102);
    EXPECT_EQ(SelectGateDomainListenPort("8103", 8102), 8103);
}

TEST(GateDomainRuntimeTest, NormalizesIoThreadCount)
{
    EXPECT_EQ(NormalizeGateDomainIoThreads(0), 4);
    EXPECT_EQ(NormalizeGateDomainIoThreads(1), 4);
    EXPECT_EQ(NormalizeGateDomainIoThreads(8), 8);
}

TEST(GateDomainRuntimeTest, SelectsConfiguredOrFallbackWorkerThreads)
{
    EXPECT_EQ(SelectGateDomainWorkerThreads("", 8), 8);
    EXPECT_EQ(SelectGateDomainWorkerThreads("0", 8), 8);
    EXPECT_EQ(SelectGateDomainWorkerThreads("6", 8), 6);
}

#include <gtest/gtest.h>

#include "random/Uuid.hpp"

#include <string>

TEST(Uuid, GeneratesVersionFourVariantOneValue)
{
    std::string value;
    std::string error;
    ASSERT_TRUE(memochat::random::GenerateUuid(value, &error)) << error;
    ASSERT_EQ(value.size(), 36U);
    EXPECT_EQ(value[8], '-');
    EXPECT_EQ(value[13], '-');
    EXPECT_EQ(value[18], '-');
    EXPECT_EQ(value[23], '-');
    EXPECT_EQ(value[14], '4');
    EXPECT_NE(std::string("89ab").find(value[19]), std::string::npos);
}

TEST(Uuid, ProducesDistinctValues)
{
    std::string first;
    std::string second;
    ASSERT_TRUE(memochat::random::GenerateUuid(first));
    ASSERT_TRUE(memochat::random::GenerateUuid(second));
    EXPECT_NE(first, second);
}

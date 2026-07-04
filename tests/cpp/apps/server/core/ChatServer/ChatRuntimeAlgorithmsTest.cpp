#include <gtest/gtest.h>

#include <string>

std::string MemoChatTestRuntimeLowerAscii(const std::string& input);
bool MemoChatTestRuntimeParseBoolFlagOr(const std::string& input, bool fallback);
int MemoChatTestRuntimeAtLeast(int value, int minimum);

TEST(ChatRuntimeAlgorithmsTest, LowercasesAsciiOnly)
{
    EXPECT_EQ(MemoChatTestRuntimeLowerAscii("INGRESS"), "ingress");
    EXPECT_EQ(MemoChatTestRuntimeLowerAscii("Worker-01"), "worker-01");
}

TEST(ChatRuntimeAlgorithmsTest, ParsesRuntimeBoolFlags)
{
    EXPECT_TRUE(MemoChatTestRuntimeParseBoolFlagOr("", true));
    EXPECT_FALSE(MemoChatTestRuntimeParseBoolFlagOr("", false));
    EXPECT_TRUE(MemoChatTestRuntimeParseBoolFlagOr("1", false));
    EXPECT_TRUE(MemoChatTestRuntimeParseBoolFlagOr("TRUE", false));
    EXPECT_TRUE(MemoChatTestRuntimeParseBoolFlagOr("yes", false));
    EXPECT_TRUE(MemoChatTestRuntimeParseBoolFlagOr("On", false));
    EXPECT_FALSE(MemoChatTestRuntimeParseBoolFlagOr("off", true));
    EXPECT_FALSE(MemoChatTestRuntimeParseBoolFlagOr("0", true));
}

TEST(ChatRuntimeAlgorithmsTest, AppliesRetryLowerBounds)
{
    EXPECT_EQ(MemoChatTestRuntimeAtLeast(50, 100), 100);
    EXPECT_EQ(MemoChatTestRuntimeAtLeast(100, 100), 100);
    EXPECT_EQ(MemoChatTestRuntimeAtLeast(5000, 100), 5000);
}

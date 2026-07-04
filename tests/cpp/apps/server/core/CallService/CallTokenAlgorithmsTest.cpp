#include <gtest/gtest.h>

#include <string>

std::string MemoChatCallTestBase64UrlEncode(const std::string& input);
unsigned long long MemoChatCallTestEncodeWithCapacity(const std::string& input, unsigned long long capacity);

TEST(CallTokenAlgorithmsTest, EncodesBase64UrlWithoutPadding)
{
    EXPECT_EQ(MemoChatCallTestBase64UrlEncode(""), "");
    EXPECT_EQ(MemoChatCallTestBase64UrlEncode("f"), "Zg");
    EXPECT_EQ(MemoChatCallTestBase64UrlEncode("fo"), "Zm8");
    EXPECT_EQ(MemoChatCallTestBase64UrlEncode("foo"), "Zm9v");
    EXPECT_EQ(MemoChatCallTestBase64UrlEncode("hello?"), "aGVsbG8_");
}

TEST(CallTokenAlgorithmsTest, RejectsInsufficientOutputCapacity)
{
    EXPECT_EQ(MemoChatCallTestEncodeWithCapacity("foo", 3), 0ULL);
    EXPECT_EQ(MemoChatCallTestEncodeWithCapacity("foo", 4), 4ULL);
}

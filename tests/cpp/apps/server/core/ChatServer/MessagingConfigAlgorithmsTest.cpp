#include <gtest/gtest.h>

#include <string>

int MemoChatTestParseMessagingIntOr(const std::string& raw, int fallback);
int MemoChatTestMessagingAtLeast(int value, int minimum);
bool MemoChatTestParseMessagingBoolOr(const std::string& raw, bool fallback);

TEST(MessagingConfigAlgorithmsTest, ParsesIntegerConfigLikeLegacyStoiFallback)
{
    EXPECT_EQ(MemoChatTestParseMessagingIntOr("", 7), 7);
    EXPECT_EQ(MemoChatTestParseMessagingIntOr("42", 7), 42);
    EXPECT_EQ(MemoChatTestParseMessagingIntOr(" 42 ", 7), 42);
    EXPECT_EQ(MemoChatTestParseMessagingIntOr("42abc", 7), 42);
    EXPECT_EQ(MemoChatTestParseMessagingIntOr("-3", 7), -3);
    EXPECT_EQ(MemoChatTestParseMessagingIntOr("bad", 7), 7);
    EXPECT_EQ(MemoChatTestParseMessagingIntOr("999999999999999999999", 7), 7);
}

TEST(MessagingConfigAlgorithmsTest, NormalizesConfiguredLowerBounds)
{
    EXPECT_EQ(MemoChatTestMessagingAtLeast(0, 1), 1);
    EXPECT_EQ(MemoChatTestMessagingAtLeast(-3, 100), 100);
    EXPECT_EQ(MemoChatTestMessagingAtLeast(128, 100), 128);
}

TEST(MessagingConfigAlgorithmsTest, ParsesKafkaBooleanFlags)
{
    EXPECT_TRUE(MemoChatTestParseMessagingBoolOr("", true));
    EXPECT_FALSE(MemoChatTestParseMessagingBoolOr("", false));
    EXPECT_TRUE(MemoChatTestParseMessagingBoolOr("1", false));
    EXPECT_TRUE(MemoChatTestParseMessagingBoolOr("TRUE", false));
    EXPECT_TRUE(MemoChatTestParseMessagingBoolOr("yes", false));
    EXPECT_TRUE(MemoChatTestParseMessagingBoolOr("On", false));
    EXPECT_FALSE(MemoChatTestParseMessagingBoolOr("off", true));
    EXPECT_FALSE(MemoChatTestParseMessagingBoolOr("0", true));
}

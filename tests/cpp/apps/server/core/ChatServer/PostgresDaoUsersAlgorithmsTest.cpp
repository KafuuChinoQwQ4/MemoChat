#include <gtest/gtest.h>

bool MemoChatTestPostgresDaoUsersAsciiDigit(char ch);
bool MemoChatTestPostgresDaoUsersNonZeroAsciiDigit(char ch);
bool MemoChatTestPostgresDaoUsersValidPublicId(const char* text, int length);
bool MemoChatTestPostgresDaoUsersPositiveUid(int uid);

TEST(PostgresDaoUsersAlgorithmsTest, ClassifiesAsciiDigitsWithoutLocale)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoUsersAsciiDigit('0'));
    EXPECT_TRUE(MemoChatTestPostgresDaoUsersAsciiDigit('9'));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsersAsciiDigit('/'));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsersAsciiDigit(':'));

    EXPECT_FALSE(MemoChatTestPostgresDaoUsersNonZeroAsciiDigit('0'));
    EXPECT_TRUE(MemoChatTestPostgresDaoUsersNonZeroAsciiDigit('1'));
    EXPECT_TRUE(MemoChatTestPostgresDaoUsersNonZeroAsciiDigit('9'));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsersNonZeroAsciiDigit('x'));
}

TEST(PostgresDaoUsersAlgorithmsTest, ValidatesUserPublicIdShape)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoUsersValidPublicId("u123456789", 10));
    EXPECT_TRUE(MemoChatTestPostgresDaoUsersValidPublicId("u900000000", 10));

    EXPECT_FALSE(MemoChatTestPostgresDaoUsersValidPublicId(nullptr, 10));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsersValidPublicId("u12345678", 9));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsersValidPublicId("x123456789", 10));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsersValidPublicId("u023456789", 10));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsersValidPublicId("u12345a789", 10));
}

TEST(PostgresDaoUsersAlgorithmsTest, AcceptsOnlyPositiveUid)
{
    EXPECT_TRUE(MemoChatTestPostgresDaoUsersPositiveUid(1));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsersPositiveUid(0));
    EXPECT_FALSE(MemoChatTestPostgresDaoUsersPositiveUid(-1));
}

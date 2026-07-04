#include <gtest/gtest.h>

namespace memochat::tests::chat::user_support
{
bool IsAsciiDigit(unsigned int byte);
bool ShouldUseCachedProfile(bool cache_hit, bool user_id_empty);
bool ShouldReportMissingUser(bool has_user);
int FriendApplyOffset();
int FriendApplyLimit();
} // namespace memochat::tests::chat::user_support

TEST(ChatUserSupportAlgorithmsTest, ClassifiesAsciiDigits)
{
    using namespace memochat::tests::chat::user_support;

    EXPECT_FALSE(IsAsciiDigit('/'));
    EXPECT_TRUE(IsAsciiDigit('0'));
    EXPECT_TRUE(IsAsciiDigit('5'));
    EXPECT_TRUE(IsAsciiDigit('9'));
    EXPECT_FALSE(IsAsciiDigit(':'));
    EXPECT_FALSE(IsAsciiDigit(0xFF));
}

TEST(ChatUserSupportAlgorithmsTest, ClassifiesCacheProfileUsability)
{
    using namespace memochat::tests::chat::user_support;

    EXPECT_FALSE(ShouldUseCachedProfile(false, false));
    EXPECT_FALSE(ShouldUseCachedProfile(false, true));
    EXPECT_FALSE(ShouldUseCachedProfile(true, true));
    EXPECT_TRUE(ShouldUseCachedProfile(true, false));
}

TEST(ChatUserSupportAlgorithmsTest, ClassifiesMissingUser)
{
    using namespace memochat::tests::chat::user_support;

    EXPECT_TRUE(ShouldReportMissingUser(false));
    EXPECT_FALSE(ShouldReportMissingUser(true));
}

TEST(ChatUserSupportAlgorithmsTest, KeepsFriendApplyPaginationWindow)
{
    using namespace memochat::tests::chat::user_support;

    EXPECT_EQ(FriendApplyOffset(), 0);
    EXPECT_EQ(FriendApplyLimit(), 10);
}

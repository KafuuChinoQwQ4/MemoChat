#include <gtest/gtest.h>

namespace memochat::tests::account::profile_support
{
int ProfileSuccessCode();
int ProfileErrorCode();
const char* MissingRequiredFieldsMessage();
const char* InvalidUidOrNickMessage();
const char* ProfileUpdateFailedMessage();
const char* InvalidUidMessage();
const char* UserNotFoundMessage();
const char* UserBaseInfoKeyPrefix();
const char* UserNameInfoKeyPrefix();
bool IsUidValid(int uid);
} // namespace memochat::tests::account::profile_support

TEST(ProfileSupportAlgorithmsTest, ResultCodesMatchCurrentValues)
{
    using namespace memochat::tests::account::profile_support;

    EXPECT_EQ(ProfileSuccessCode(), 0);
    EXPECT_EQ(ProfileErrorCode(), 1);
}

TEST(ProfileSupportAlgorithmsTest, ErrorMessagesArePinned)
{
    using namespace memochat::tests::account::profile_support;

    EXPECT_STREQ(MissingRequiredFieldsMessage(), "missing required fields");
    EXPECT_STREQ(InvalidUidOrNickMessage(), "invalid uid or nick");
    EXPECT_STREQ(ProfileUpdateFailedMessage(), "profile update failed");
    EXPECT_STREQ(InvalidUidMessage(), "invalid uid");
    EXPECT_STREQ(UserNotFoundMessage(), "user not found");
}

TEST(ProfileSupportAlgorithmsTest, RedisKeyPrefixesArePinned)
{
    using namespace memochat::tests::account::profile_support;

    EXPECT_STREQ(UserBaseInfoKeyPrefix(), "ubaseinfo_");
    EXPECT_STREQ(UserNameInfoKeyPrefix(), "nameinfo_");
}

TEST(ProfileSupportAlgorithmsTest, UidGuardRejectsNonPositive)
{
    using namespace memochat::tests::account::profile_support;

    EXPECT_FALSE(IsUidValid(0));
    EXPECT_FALSE(IsUidValid(-1));
    EXPECT_TRUE(IsUidValid(1));
    EXPECT_TRUE(IsUidValid(100));
}

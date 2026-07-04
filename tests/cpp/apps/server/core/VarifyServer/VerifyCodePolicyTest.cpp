#include <gtest/gtest.h>

#include "VerifyCodePolicy.hpp"

#include <algorithm>
#include <cctype>

namespace
{

bool IsAllDigits(const std::string& value)
{
    return std::all_of(value.begin(),
                       value.end(),
                       [](unsigned char ch)
                       {
                           return std::isdigit(ch) != 0;
                       });
}

} // namespace

TEST(VerifyCodePolicyTest, BuildsCanonicalRedisKeyWithCodePrefix)
{
    EXPECT_EQ(varifyservice::BuildVerifyCodeKey("user@example.com"), "code_user@example.com");
}

TEST(VerifyCodePolicyTest, SyntheticLoadtestEmailIsCaseInsensitive)
{
    EXPECT_TRUE(varifyservice::IsSyntheticLoadtestEmail("case@loadtest.local"));
    EXPECT_TRUE(varifyservice::IsSyntheticLoadtestEmail("CASE@LOADTEST.LOCAL"));
    EXPECT_FALSE(varifyservice::IsSyntheticLoadtestEmail("case@example.com"));
}

TEST(VerifyCodePolicyTest, ValidatesRegistrationEmailFormat)
{
    EXPECT_TRUE(varifyservice::IsValidVerifyEmail("valid.user+1@example.com"));
    EXPECT_TRUE(varifyservice::IsValidVerifyEmail("user_name-1@mail.example.org"));

    EXPECT_FALSE(varifyservice::IsValidVerifyEmail(""));
    EXPECT_FALSE(varifyservice::IsValidVerifyEmail("missing-at.example.com"));
    EXPECT_FALSE(varifyservice::IsValidVerifyEmail("missing-domain@"));
    EXPECT_FALSE(varifyservice::IsValidVerifyEmail("bad space@example.com"));
}

TEST(VerifyCodePolicyTest, GeneratesNumericVerifyCodeWithRequestedLength)
{
    const std::string code = varifyservice::GenerateNumericVerifyCode(6);

    ASSERT_EQ(code.size(), 6);
    EXPECT_TRUE(IsAllDigits(code));
}

TEST(VerifyCodePolicyTest, ClampsInvalidVerifyCodeLengthToConfiguredDefault)
{
    const std::string zero_length_code = varifyservice::GenerateNumericVerifyCode(0);
    const std::string too_long_code = varifyservice::GenerateNumericVerifyCode(32);

    EXPECT_EQ(zero_length_code.size(), 6);
    EXPECT_EQ(too_long_code.size(), 6);
    EXPECT_TRUE(IsAllDigits(zero_length_code));
    EXPECT_TRUE(IsAllDigits(too_long_code));
}

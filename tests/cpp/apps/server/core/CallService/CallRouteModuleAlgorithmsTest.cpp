#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace memochat::tests::call
{
const char* GetMethod();
const char* PostMethod();
const char* StartPath();
const char* AcceptPath();
const char* RejectPath();
const char* CancelPath();
const char* HangupPath();
const char* TokenPath();
int ParseRouteUid(const char* data, unsigned long long size);
bool ShouldAppendTraceId(bool body_empty, char last_char, bool already_has_trace_id);
} // namespace memochat::tests::call

TEST(CallRouteModuleAlgorithmsTest, ExposesStableHttpMethods)
{
    EXPECT_STREQ(memochat::tests::call::GetMethod(), "GET");
    EXPECT_STREQ(memochat::tests::call::PostMethod(), "POST");
}

TEST(CallRouteModuleAlgorithmsTest, ExposesCallGatewayRouteRegistrationPaths)
{
    using memochat::tests::call::AcceptPath;
    using memochat::tests::call::CancelPath;
    using memochat::tests::call::HangupPath;
    using memochat::tests::call::RejectPath;
    using memochat::tests::call::StartPath;
    using memochat::tests::call::TokenPath;

    constexpr std::array<std::string_view, 6> expected = {
        "/api/call/start",
        "/api/call/accept",
        "/api/call/reject",
        "/api/call/cancel",
        "/api/call/hangup",
        "/api/call/token",
    };
    const std::array<std::string_view, 6> actual = {
        StartPath(),
        AcceptPath(),
        RejectPath(),
        CancelPath(),
        HangupPath(),
        TokenPath(),
    };

    EXPECT_EQ(actual, expected);
}

TEST(CallRouteModuleAlgorithmsTest, ParsesUnsignedDecimalUidPrefixAndRejectsOverflow)
{
    EXPECT_EQ(memochat::tests::call::ParseRouteUid("42", 2), 42);
    EXPECT_EQ(memochat::tests::call::ParseRouteUid("42abc", 5), 42);
    EXPECT_EQ(memochat::tests::call::ParseRouteUid("", 0), 0);
    EXPECT_EQ(memochat::tests::call::ParseRouteUid("abc", 3), 0);
    EXPECT_EQ(memochat::tests::call::ParseRouteUid("2147483648", 10), 0);
}

TEST(CallRouteModuleAlgorithmsTest, AppendsTraceIdOnlyForJsonObjectWithoutTraceId)
{
    EXPECT_TRUE(memochat::tests::call::ShouldAppendTraceId(false, '}', false));
    EXPECT_FALSE(memochat::tests::call::ShouldAppendTraceId(false, '}', true));
    EXPECT_FALSE(memochat::tests::call::ShouldAppendTraceId(false, ']', false));
    EXPECT_FALSE(memochat::tests::call::ShouldAppendTraceId(true, '\0', false));
}

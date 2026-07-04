#include <gtest/gtest.h>

#include <string>

namespace memochat::tests::gate::h3_json_support
{
std::string NullConnectionMessage();
std::string EmptyBodyMessage();
std::string JsonParseErrorPrefix();
std::string ResponseContentType();
int ErrorCode();
int SuccessCode();
int BadRequestStatus();
int OkStatus();
int InternalErrorStatus();
bool ShouldForceErrorCode(bool has_error_member, int current_error);
} // namespace memochat::tests::gate::h3_json_support

TEST(GateHttp3JsonSupportAlgorithmsTest, ErrorMessagesAreStable)
{
    EXPECT_EQ(memochat::tests::gate::h3_json_support::NullConnectionMessage(), "null connection");
    EXPECT_EQ(memochat::tests::gate::h3_json_support::EmptyBodyMessage(), "empty request body");
    EXPECT_EQ(memochat::tests::gate::h3_json_support::JsonParseErrorPrefix(), "json parse error: ");
}

TEST(GateHttp3JsonSupportAlgorithmsTest, ResponseContentTypeIsJson)
{
    EXPECT_EQ(memochat::tests::gate::h3_json_support::ResponseContentType(), "application/json");
}

TEST(GateHttp3JsonSupportAlgorithmsTest, ErrorAndSuccessCodes)
{
    EXPECT_EQ(memochat::tests::gate::h3_json_support::ErrorCode(), 1);
    EXPECT_EQ(memochat::tests::gate::h3_json_support::SuccessCode(), 0);
}

TEST(GateHttp3JsonSupportAlgorithmsTest, StatusCodes)
{
    EXPECT_EQ(memochat::tests::gate::h3_json_support::BadRequestStatus(), 400);
    EXPECT_EQ(memochat::tests::gate::h3_json_support::OkStatus(), 200);
    EXPECT_EQ(memochat::tests::gate::h3_json_support::InternalErrorStatus(), 500);
}

TEST(GateHttp3JsonSupportAlgorithmsTest, ForcesErrorCodeOnlyWhenMissingOrZero)
{
    EXPECT_TRUE(memochat::tests::gate::h3_json_support::ShouldForceErrorCode(false, 0));
    EXPECT_TRUE(memochat::tests::gate::h3_json_support::ShouldForceErrorCode(true, 0));
    EXPECT_FALSE(memochat::tests::gate::h3_json_support::ShouldForceErrorCode(true, 1));
    EXPECT_FALSE(memochat::tests::gate::h3_json_support::ShouldForceErrorCode(true, 5));
}

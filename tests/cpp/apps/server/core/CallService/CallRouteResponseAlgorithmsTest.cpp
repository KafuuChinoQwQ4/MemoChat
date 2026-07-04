#include <gtest/gtest.h>

#include <string>

namespace memochat::tests::call::route_response
{
int OkStatus();
const char* JsonContentType();
const char* TokenJsonContentType();
const char* TraceIdSearchToken();
} // namespace memochat::tests::call::route_response

namespace
{
using namespace memochat::tests::call::route_response;
} // namespace

TEST(CallRouteResponseAlgorithmsTest, PinsOkStatus)
{
    EXPECT_EQ(OkStatus(), 200);
}

TEST(CallRouteResponseAlgorithmsTest, PinsJsonContentTypes)
{
    EXPECT_EQ(std::string(JsonContentType()), "application/json");
    EXPECT_EQ(std::string(TokenJsonContentType()), "text/json");
}

TEST(CallRouteResponseAlgorithmsTest, PinsTraceIdSearchToken)
{
    EXPECT_EQ(std::string(TraceIdSearchToken()), "\"trace_id\"");
}

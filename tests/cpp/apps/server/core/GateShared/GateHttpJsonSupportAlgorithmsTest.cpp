#include <gtest/gtest.h>

namespace memochat::tests::gate::http_json
{
const char* JsonContentType();
const char* EmptyTraceId();
bool ShouldWriteJsonParseError(bool parsed_json);
bool ShouldAttachTraceId(bool parsed_json);
} // namespace memochat::tests::gate::http_json

TEST(GateHttpJsonSupportAlgorithmsTest, ExposesJsonResponseMetadata)
{
    EXPECT_STREQ(memochat::tests::gate::http_json::JsonContentType(), "application/json");
    EXPECT_STREQ(memochat::tests::gate::http_json::EmptyTraceId(), "");
}

TEST(GateHttpJsonSupportAlgorithmsTest, WritesParseErrorOnlyWhenParsingFails)
{
    EXPECT_TRUE(memochat::tests::gate::http_json::ShouldWriteJsonParseError(false));
    EXPECT_FALSE(memochat::tests::gate::http_json::ShouldWriteJsonParseError(true));
}

TEST(GateHttpJsonSupportAlgorithmsTest, AttachesTraceIdOnlyAfterSuccessfulParse)
{
    EXPECT_FALSE(memochat::tests::gate::http_json::ShouldAttachTraceId(false));
    EXPECT_TRUE(memochat::tests::gate::http_json::ShouldAttachTraceId(true));
}

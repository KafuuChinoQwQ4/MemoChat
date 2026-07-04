#include "routing/RouteSchema.hpp"

#include <gtest/gtest.h>

#include <string>

namespace
{

struct SampleRequestDto
{
    int uid = 0;
    std::string media_id;
    bool include_deleted = false;
};

struct SampleResponseDto
{
    int error = 0;
    std::string media_url;
};

struct RenamedRequestDto
{
    int uid = 0;
    bool delete_ = false;
    std::string content;
};

struct RenamedResponseDto
{
    int error = 0;
    bool delete_ = false;
};

} // namespace

TEST(RouteSchemaTest, BuildsRequestAndResponseFieldSchemasFromDtoFields)
{
    const auto schema =
        memochat::gate::routing::MakeRouteSchema<SampleRequestDto, SampleResponseDto>("POST",
                                                                                      "/media/status",
                                                                                      "media.status",
                                                                                      "SampleRequestDto",
                                                                                      "SampleResponseDto");

    EXPECT_EQ(schema.method, "POST");
    EXPECT_EQ(schema.path, "/media/status");
    EXPECT_EQ(schema.name, "media.status");

    EXPECT_EQ(schema.request.role, "request");
    EXPECT_EQ(schema.request.type_name, "SampleRequestDto");
    ASSERT_EQ(schema.request.fields.size(), 3U);
    EXPECT_EQ(schema.request.fields[0].name, "uid");
    EXPECT_EQ(schema.request.fields[1].name, "media_id");
    EXPECT_EQ(schema.request.fields[2].name, "include_deleted");

    EXPECT_EQ(schema.response.role, "response");
    EXPECT_EQ(schema.response.type_name, "SampleResponseDto");
    ASSERT_EQ(schema.response.fields.size(), 2U);
    EXPECT_EQ(schema.response.fields[0].name, "error");
    EXPECT_EQ(schema.response.fields[1].name, "media_url");
}

TEST(RouteSchemaTest, AppliesExplicitFieldNameOverridesWithoutChangingFieldOrder)
{
    const auto default_schema =
        memochat::gate::routing::MakeRouteSchema<RenamedRequestDto, RenamedResponseDto>("POST",
                                                                                        "/moments/comment",
                                                                                        "moments.comment",
                                                                                        "RenamedRequestDto",
                                                                                        "RenamedResponseDto");

    ASSERT_EQ(default_schema.request.fields.size(), 3U);
    EXPECT_EQ(default_schema.request.fields[0].name, "uid");
    EXPECT_EQ(default_schema.request.fields[1].name, "delete_");
    EXPECT_EQ(default_schema.request.fields[2].name, "content");
    ASSERT_EQ(default_schema.response.fields.size(), 2U);
    EXPECT_EQ(default_schema.response.fields[0].name, "error");
    EXPECT_EQ(default_schema.response.fields[1].name, "delete_");

    const auto schema =
        memochat::gate::routing::MakeRouteSchema<RenamedRequestDto, RenamedResponseDto>("POST",
                                                                                        "/moments/comment",
                                                                                        "moments.comment",
                                                                                        "RenamedRequestDto",
                                                                                        "RenamedResponseDto",
                                                                                        {{"delete_", "delete"}},
                                                                                        {{"delete_", "delete"}});

    ASSERT_EQ(schema.request.fields.size(), 3U);
    EXPECT_EQ(schema.request.fields[0].name, "uid");
    EXPECT_EQ(schema.request.fields[1].name, "delete");
    EXPECT_EQ(schema.request.fields[2].name, "content");
    ASSERT_EQ(schema.response.fields.size(), 2U);
    EXPECT_EQ(schema.response.fields[0].name, "error");
    EXPECT_EQ(schema.response.fields[1].name, "delete");
}

TEST(RouteSchemaTest, BuildsNoBodyRouteSchema)
{
    const auto schema = memochat::gate::routing::MakeRouteSchemaWithoutBody("GET", "/health", "health.check");

    EXPECT_EQ(schema.method, "GET");
    EXPECT_EQ(schema.path, "/health");
    EXPECT_EQ(schema.name, "health.check");
    EXPECT_EQ(schema.request.role, "request");
    EXPECT_TRUE(schema.request.type_name.empty());
    EXPECT_TRUE(schema.request.fields.empty());
    EXPECT_EQ(schema.response.role, "response");
    EXPECT_TRUE(schema.response.type_name.empty());
    EXPECT_TRUE(schema.response.fields.empty());
}

TEST(RouteSchemaTest, RendersDeterministicRouteSchemaSnapshot)
{
    const auto route =
        memochat::gate::routing::MakeRouteSchema<SampleRequestDto, SampleResponseDto>("POST",
                                                                                      "/media/status",
                                                                                      "media.status",
                                                                                      "SampleRequestDto",
                                                                                      "SampleResponseDto");
    const auto health = memochat::gate::routing::MakeRouteSchemaWithoutBody("GET", "/health", "health.check");

    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot({route, health});

    EXPECT_EQ(snapshot,
              "route: media.status\n"
              "method: POST\n"
              "path: /media/status\n"
              "request: SampleRequestDto\n"
              "  - uid\n"
              "  - media_id\n"
              "  - include_deleted\n"
              "response: SampleResponseDto\n"
              "  - error\n"
              "  - media_url\n"
              "\n"
              "route: health.check\n"
              "method: GET\n"
              "path: /health\n"
              "request: <none>\n"
              "response: <none>\n"
              "\n");
}

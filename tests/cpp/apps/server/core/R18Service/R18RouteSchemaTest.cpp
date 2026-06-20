#include "modules/r18/R18RouteModule.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{

void ExpectFields(const memochat::gate::routing::RouteBodySchema& schema, const std::vector<std::string>& names)
{
    ASSERT_EQ(schema.fields.size(), names.size());
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        EXPECT_EQ(schema.fields[i].name, names[i]);
    }
}

const char* ExpectedR18RouteSchemaSnapshot()
{
    return "route: r18.source.enable\n"
           "method: POST\n"
           "path: /api/r18/source/enable\n"
           "request: R18SourceToggleRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - source_id\n"
           "response: R18SourceToggleResponseDto\n"
           "  - source_id\n"
           "  - enabled\n"
           "\n"
           "route: r18.source.disable\n"
           "method: POST\n"
           "path: /api/r18/source/disable\n"
           "request: R18SourceToggleRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - source_id\n"
           "response: R18SourceToggleResponseDto\n"
           "  - source_id\n"
           "  - enabled\n"
           "\n"
           "route: r18.favorite.toggle\n"
           "method: POST\n"
           "path: /api/r18/favorite/toggle\n"
           "request: R18FavoriteToggleRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - source_id\n"
           "  - comic_id\n"
           "  - favorited\n"
           "response: R18FavoriteToggleResponseDto\n"
           "  - source_id\n"
           "  - comic_id\n"
           "  - favorited\n"
           "\n"
           "route: r18.history.update\n"
           "method: POST\n"
           "path: /api/r18/history/update\n"
           "request: R18HistoryUpdateRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - source_id\n"
           "  - comic_id\n"
           "  - chapter_id\n"
           "  - page_index\n"
           "response: R18HistoryUpdateResponseDto\n"
           "  - source_id\n"
           "  - comic_id\n"
           "  - chapter_id\n"
           "  - page_index\n"
           "\n";
}

} // namespace

TEST(R18RouteSchemaTest, ListsOnlyStableR18ActionRoutes)
{
    const auto schemas = memochat::gate::modules::r18::R18RouteModule::RouteSchemas();

    ASSERT_EQ(schemas.size(), 4U);
    EXPECT_EQ(schemas[0].name, "r18.source.enable");
    EXPECT_EQ(schemas[0].method, "POST");
    EXPECT_EQ(schemas[0].path, "/api/r18/source/enable");
    EXPECT_EQ(schemas[0].request.type_name, "R18SourceToggleRequestDto");
    EXPECT_EQ(schemas[0].response.type_name, "R18SourceToggleResponseDto");

    EXPECT_EQ(schemas[1].name, "r18.source.disable");
    EXPECT_EQ(schemas[1].method, "POST");
    EXPECT_EQ(schemas[1].path, "/api/r18/source/disable");
    EXPECT_EQ(schemas[1].request.type_name, "R18SourceToggleRequestDto");
    EXPECT_EQ(schemas[1].response.type_name, "R18SourceToggleResponseDto");

    EXPECT_EQ(schemas[2].name, "r18.favorite.toggle");
    EXPECT_EQ(schemas[2].method, "POST");
    EXPECT_EQ(schemas[2].path, "/api/r18/favorite/toggle");
    EXPECT_EQ(schemas[2].request.type_name, "R18FavoriteToggleRequestDto");
    EXPECT_EQ(schemas[2].response.type_name, "R18FavoriteToggleResponseDto");

    EXPECT_EQ(schemas[3].name, "r18.history.update");
    EXPECT_EQ(schemas[3].method, "POST");
    EXPECT_EQ(schemas[3].path, "/api/r18/history/update");
    EXPECT_EQ(schemas[3].request.type_name, "R18HistoryUpdateRequestDto");
    EXPECT_EQ(schemas[3].response.type_name, "R18HistoryUpdateResponseDto");
}

TEST(R18RouteSchemaTest, BuildsFieldInventoriesFromR18Dtos)
{
    const auto schemas = memochat::gate::modules::r18::R18RouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 4U);

    ExpectFields(schemas[0].request, {"uid", "token", "source_id"});
    ExpectFields(schemas[0].response, {"source_id", "enabled"});

    ExpectFields(schemas[1].request, {"uid", "token", "source_id"});
    ExpectFields(schemas[1].response, {"source_id", "enabled"});

    ExpectFields(schemas[2].request, {"uid", "token", "source_id", "comic_id", "favorited"});
    ExpectFields(schemas[2].response, {"source_id", "comic_id", "favorited"});

    ExpectFields(schemas[3].request, {"uid", "token", "source_id", "comic_id", "chapter_id", "page_index"});
    ExpectFields(schemas[3].response, {"source_id", "comic_id", "chapter_id", "page_index"});
}

TEST(R18RouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::r18::R18RouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedR18RouteSchemaSnapshot());
}

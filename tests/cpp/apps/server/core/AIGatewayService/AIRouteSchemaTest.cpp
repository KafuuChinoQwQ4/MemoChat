#include "modules/ai/AIRouteModule.h"

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

const char* ExpectedAIRouteSchemaSnapshot()
{
    return "route: ai.model.api.register\n"
           "method: POST\n"
           "path: /ai/model/api/register\n"
           "request: AIRegisterApiProviderRequestDto\n"
           "  - provider_name\n"
           "  - base_url\n"
           "  - api_key\n"
           "  - adapter\n"
           "response: AIRegisterApiProviderResponseDto\n"
           "  - code\n"
           "  - message\n"
           "  - provider_id\n"
           "  - models\n"
           "\n"
           "route: ai.model.api.delete\n"
           "method: POST\n"
           "path: /ai/model/api/delete\n"
           "request: AIDeleteApiProviderRequestDto\n"
           "  - provider_id\n"
           "response: AIDeleteApiProviderResponseDto\n"
           "  - code\n"
           "  - message\n"
           "  - provider_id\n"
           "\n"
           "route: ai.kb.upload\n"
           "method: POST\n"
           "path: /ai/kb/upload\n"
           "request: AIKbUploadRequestDto\n"
           "  - uid\n"
           "  - file_name\n"
           "  - file_type\n"
           "  - content\n"
           "response: AIKbUploadResponseDto\n"
           "  - code\n"
           "  - message\n"
           "  - chunks\n"
           "  - kb_id\n"
           "\n"
           "route: ai.kb.search\n"
           "method: POST\n"
           "path: /ai/kb/search\n"
           "request: AIKbSearchRequestDto\n"
           "  - uid\n"
           "  - query\n"
           "  - top_k\n"
           "response: AIKbSearchResponseDto\n"
           "  - code\n"
           "  - chunks\n"
           "\n"
           "route: ai.kb.delete\n"
           "method: POST\n"
           "path: /ai/kb/delete\n"
           "request: AIKbDeleteRequestDto\n"
           "  - uid\n"
           "  - kb_id\n"
           "response: AISimpleResponseDto\n"
           "  - code\n"
           "  - message\n"
           "\n";
}

} // namespace

TEST(AIRouteSchemaTest, ListsOnlyStableAIGatewayMapperRoutes)
{
    const auto schemas = memochat::gate::modules::ai::AIRouteModule::RouteSchemas();

    ASSERT_EQ(schemas.size(), 5U);
    EXPECT_EQ(schemas[0].name, "ai.model.api.register");
    EXPECT_EQ(schemas[0].method, "POST");
    EXPECT_EQ(schemas[0].path, "/ai/model/api/register");
    EXPECT_EQ(schemas[0].request.type_name, "AIRegisterApiProviderRequestDto");
    EXPECT_EQ(schemas[0].response.type_name, "AIRegisterApiProviderResponseDto");

    EXPECT_EQ(schemas[1].name, "ai.model.api.delete");
    EXPECT_EQ(schemas[1].method, "POST");
    EXPECT_EQ(schemas[1].path, "/ai/model/api/delete");
    EXPECT_EQ(schemas[1].request.type_name, "AIDeleteApiProviderRequestDto");
    EXPECT_EQ(schemas[1].response.type_name, "AIDeleteApiProviderResponseDto");

    EXPECT_EQ(schemas[2].name, "ai.kb.upload");
    EXPECT_EQ(schemas[2].method, "POST");
    EXPECT_EQ(schemas[2].path, "/ai/kb/upload");
    EXPECT_EQ(schemas[2].request.type_name, "AIKbUploadRequestDto");
    EXPECT_EQ(schemas[2].response.type_name, "AIKbUploadResponseDto");

    EXPECT_EQ(schemas[3].name, "ai.kb.search");
    EXPECT_EQ(schemas[3].method, "POST");
    EXPECT_EQ(schemas[3].path, "/ai/kb/search");
    EXPECT_EQ(schemas[3].request.type_name, "AIKbSearchRequestDto");
    EXPECT_EQ(schemas[3].response.type_name, "AIKbSearchResponseDto");

    EXPECT_EQ(schemas[4].name, "ai.kb.delete");
    EXPECT_EQ(schemas[4].method, "POST");
    EXPECT_EQ(schemas[4].path, "/ai/kb/delete");
    EXPECT_EQ(schemas[4].request.type_name, "AIKbDeleteRequestDto");
    EXPECT_EQ(schemas[4].response.type_name, "AISimpleResponseDto");
}

TEST(AIRouteSchemaTest, BuildsFieldInventoriesFromAIGatewayDtos)
{
    const auto schemas = memochat::gate::modules::ai::AIRouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 5U);

    ExpectFields(schemas[0].request, {"provider_name", "base_url", "api_key", "adapter"});
    ExpectFields(schemas[0].response, {"code", "message", "provider_id", "models"});

    ExpectFields(schemas[1].request, {"provider_id"});
    ExpectFields(schemas[1].response, {"code", "message", "provider_id"});

    ExpectFields(schemas[2].request, {"uid", "file_name", "file_type", "content"});
    ExpectFields(schemas[2].response, {"code", "message", "chunks", "kb_id"});

    ExpectFields(schemas[3].request, {"uid", "query", "top_k"});
    ExpectFields(schemas[3].response, {"code", "chunks"});

    ExpectFields(schemas[4].request, {"uid", "kb_id"});
    ExpectFields(schemas[4].response, {"code", "message"});
}

TEST(AIRouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::ai::AIRouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedAIRouteSchemaSnapshot());
}

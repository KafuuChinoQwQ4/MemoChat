#include "modules/ai/AIRouteModule.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace memochat::tests::ai::route_schema
{
const char* PostMethod();
const char* RegisterApiProviderPath();
const char* RegisterApiProviderRouteName();
const char* RegisterApiProviderRequestTypeName();
const char* RegisterApiProviderResponseTypeName();
const char* DeleteApiProviderPath();
const char* DeleteApiProviderRouteName();
const char* DeleteApiProviderRequestTypeName();
const char* DeleteApiProviderResponseTypeName();
const char* KbUploadPath();
const char* KbUploadRouteName();
const char* KbUploadRequestTypeName();
const char* KbUploadResponseTypeName();
const char* KbSearchPath();
const char* KbSearchRouteName();
const char* KbSearchRequestTypeName();
const char* KbSearchResponseTypeName();
const char* KbDeletePath();
const char* KbDeleteRouteName();
const char* KbDeleteRequestTypeName();
const char* SimpleResponseTypeName();
} // namespace memochat::tests::ai::route_schema

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
    EXPECT_EQ(schemas[0].name, memochat::tests::ai::route_schema::RegisterApiProviderRouteName());
    EXPECT_EQ(schemas[0].method, memochat::tests::ai::route_schema::PostMethod());
    EXPECT_EQ(schemas[0].path, memochat::tests::ai::route_schema::RegisterApiProviderPath());
    EXPECT_EQ(schemas[0].request.type_name, memochat::tests::ai::route_schema::RegisterApiProviderRequestTypeName());
    EXPECT_EQ(schemas[0].response.type_name, memochat::tests::ai::route_schema::RegisterApiProviderResponseTypeName());

    EXPECT_EQ(schemas[1].name, memochat::tests::ai::route_schema::DeleteApiProviderRouteName());
    EXPECT_EQ(schemas[1].method, memochat::tests::ai::route_schema::PostMethod());
    EXPECT_EQ(schemas[1].path, memochat::tests::ai::route_schema::DeleteApiProviderPath());
    EXPECT_EQ(schemas[1].request.type_name, memochat::tests::ai::route_schema::DeleteApiProviderRequestTypeName());
    EXPECT_EQ(schemas[1].response.type_name, memochat::tests::ai::route_schema::DeleteApiProviderResponseTypeName());

    EXPECT_EQ(schemas[2].name, memochat::tests::ai::route_schema::KbUploadRouteName());
    EXPECT_EQ(schemas[2].method, memochat::tests::ai::route_schema::PostMethod());
    EXPECT_EQ(schemas[2].path, memochat::tests::ai::route_schema::KbUploadPath());
    EXPECT_EQ(schemas[2].request.type_name, memochat::tests::ai::route_schema::KbUploadRequestTypeName());
    EXPECT_EQ(schemas[2].response.type_name, memochat::tests::ai::route_schema::KbUploadResponseTypeName());

    EXPECT_EQ(schemas[3].name, memochat::tests::ai::route_schema::KbSearchRouteName());
    EXPECT_EQ(schemas[3].method, memochat::tests::ai::route_schema::PostMethod());
    EXPECT_EQ(schemas[3].path, memochat::tests::ai::route_schema::KbSearchPath());
    EXPECT_EQ(schemas[3].request.type_name, memochat::tests::ai::route_schema::KbSearchRequestTypeName());
    EXPECT_EQ(schemas[3].response.type_name, memochat::tests::ai::route_schema::KbSearchResponseTypeName());

    EXPECT_EQ(schemas[4].name, memochat::tests::ai::route_schema::KbDeleteRouteName());
    EXPECT_EQ(schemas[4].method, memochat::tests::ai::route_schema::PostMethod());
    EXPECT_EQ(schemas[4].path, memochat::tests::ai::route_schema::KbDeletePath());
    EXPECT_EQ(schemas[4].request.type_name, memochat::tests::ai::route_schema::KbDeleteRequestTypeName());
    EXPECT_EQ(schemas[4].response.type_name, memochat::tests::ai::route_schema::SimpleResponseTypeName());
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

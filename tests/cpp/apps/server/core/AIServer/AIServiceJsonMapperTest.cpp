#include "AIServiceJsonMapper.hpp"

#include "AIServiceJsonDtos.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<ai_service_json_mapper::AIModelInfoJsonDto>(
    std::array<std::string_view,
               6>{"model_type", "model_name", "display_name", "is_enabled", "context_window", "supports_thinking"}));
static_assert(memochat::reflection::FieldNamesEqual<ai_service_json_mapper::AIModelListJsonDto>(
    std::array<std::string_view, 4>{"code", "models", "default_model", "has_default_model"}));
static_assert(memochat::reflection::FieldNamesEqual<ai_service_json_mapper::AIRegisterApiProviderJsonDto>(
    std::array<std::string_view, 4>{"code", "message", "provider_id", "models"}));
static_assert(memochat::reflection::FieldNamesEqual<ai_service_json_mapper::AIKnowledgeBaseInfoJsonDto>(
    std::array<std::string_view, 5>{"kb_id", "name", "chunk_count", "created_at", "status"}));
static_assert(memochat::reflection::FieldNamesEqual<ai_service_json_mapper::AIKnowledgeBaseListJsonDto>(
    std::array<std::string_view, 2>{"code", "knowledge_bases"}));
static_assert(memochat::reflection::FieldNamesEqual<ai_service_json_mapper::AIKbDeleteJsonDto>(
    std::array<std::string_view, 2>{"code", "message"}));
#endif

namespace
{

memochat::json::JsonValue ParseJson(std::string_view payload)
{
    memochat::json::JsonValue value;
    EXPECT_TRUE(memochat::json::reader_parse(payload, value));
    return value;
}

} // namespace

TEST(AIServiceJsonMapperTest, PopulatesModelListAndExplicitDefaultModel)
{
    auto payload = ParseJson(R"json(
        {
          "code": 0,
          "models": [
            {
              "model_type": "ollama",
              "model_name": "qwen3:4b",
              "display_name": "Qwen 3 4B",
              "is_enabled": true,
              "context_window": 8192,
              "supports_thinking": true
            },
            {
              "model_type": "openai",
              "model_name": "gpt-4o-mini",
              "display_name": "GPT-4o Mini",
              "is_enabled": false,
              "context_window": 128000,
              "supports_thinking": false
            }
          ],
          "default_model": {
            "model_type": "ollama",
            "model_name": "qwen3:4b",
            "display_name": "Qwen 3 4B",
            "is_enabled": true,
            "context_window": 8192,
            "supports_thinking": true
          }
        }
    )json");

    ai::AIListModelsRsp reply;
    EXPECT_TRUE(ai_service_json_mapper::PopulateModelListFromJson(payload, &reply));

    ASSERT_EQ(reply.code(), 0);
    ASSERT_EQ(reply.models_size(), 2);
    EXPECT_EQ(reply.models(0).model_type(), "ollama");
    EXPECT_EQ(reply.models(0).model_name(), "qwen3:4b");
    EXPECT_TRUE(reply.models(0).supports_thinking());
    EXPECT_EQ(reply.models(1).display_name(), "GPT-4o Mini");
    EXPECT_FALSE(reply.models(1).is_enabled());
    EXPECT_FALSE(reply.models(1).supports_thinking());
    EXPECT_EQ(reply.default_model().model_name(), "qwen3:4b");
    EXPECT_EQ(reply.default_model().display_name(), "Qwen 3 4B");
    EXPECT_TRUE(reply.default_model().supports_thinking());
}

TEST(AIServiceJsonMapperTest, FallsBackToFirstModelWhenDefaultModelMissing)
{
    auto payload = ParseJson(R"json(
        {
          "code": 0,
          "models": [
            {
              "model_type": "ollama",
              "model_name": "qwen3:4b",
              "display_name": "Qwen 3 4B",
              "is_enabled": true,
              "context_window": 8192
            }
          ]
        }
    )json");

    ai::AIListModelsRsp reply;
    EXPECT_TRUE(ai_service_json_mapper::PopulateModelListFromJson(payload, &reply));

    ASSERT_EQ(reply.models_size(), 1);
    EXPECT_EQ(reply.default_model().model_name(), "qwen3:4b");
    EXPECT_EQ(reply.default_model().display_name(), "Qwen 3 4B");
}

TEST(AIServiceJsonMapperTest, AcceptsEmptyModelList)
{
    auto payload = ParseJson(R"json(
        {
          "code": 0,
          "models": [],
          "default_model": null
        }
    )json");

    ai::AIListModelsRsp reply;
    EXPECT_TRUE(ai_service_json_mapper::PopulateModelListFromJson(payload, &reply));

    EXPECT_EQ(reply.code(), 0);
    EXPECT_EQ(reply.models_size(), 0);
    EXPECT_FALSE(reply.has_default_model());
}

TEST(AIServiceJsonMapperTest, PreservesLegacyModelDefaultsForWrongTypes)
{
    auto payload = ParseJson(R"json(
        {
          "code": 0,
          "models": [
            {
              "model_type": "ollama",
              "model_name": "qwen3:4b",
              "display_name": 7,
              "is_enabled": "bad",
              "context_window": "bad",
              "supports_thinking": "bad"
            }
          ],
          "default_model": []
        }
    )json");

    ai::AIListModelsRsp reply;
    EXPECT_TRUE(ai_service_json_mapper::PopulateModelListFromJson(payload, &reply));

    ASSERT_EQ(reply.models_size(), 1);
    EXPECT_EQ(reply.models(0).model_name(), "qwen3:4b");
    EXPECT_TRUE(reply.models(0).display_name().empty());
    EXPECT_TRUE(reply.models(0).is_enabled());
    EXPECT_EQ(reply.models(0).context_window(), 0);
    EXPECT_FALSE(reply.models(0).supports_thinking());
    EXPECT_EQ(reply.default_model().model_name(), "qwen3:4b");
}

TEST(AIServiceJsonMapperTest, PopulatesRegisteredApiProviderModels)
{
    auto payload = ParseJson(R"json(
        {
          "code": 0,
          "message": "ok",
          "provider_id": "api-gpt",
          "models": [
            {
              "model_type": "api-gpt",
              "model_name": "gpt-4o-mini",
              "display_name": "gpt-4o-mini",
              "is_enabled": true,
              "context_window": 0,
              "supports_thinking": false
            }
          ]
        }
    )json");

    ai::AIRegisterApiProviderRsp reply;
    ai_service_json_mapper::PopulateRegisterApiProviderFromJson(payload, &reply);

    EXPECT_EQ(reply.code(), 0);
    EXPECT_EQ(reply.message(), "ok");
    EXPECT_EQ(reply.provider_id(), "api-gpt");
    ASSERT_EQ(reply.models_size(), 1);
    EXPECT_EQ(reply.models(0).model_type(), "api-gpt");
    EXPECT_EQ(reply.models(0).model_name(), "gpt-4o-mini");
    EXPECT_FALSE(reply.models(0).supports_thinking());
}

TEST(AIServiceJsonMapperTest, RegisterProviderKeepsLegacyMissingFieldDefaults)
{
    auto payload = ParseJson(R"json({})json");

    ai::AIRegisterApiProviderRsp reply;
    ai_service_json_mapper::PopulateRegisterApiProviderFromJson(payload, &reply);

    EXPECT_EQ(reply.code(), 0);
    EXPECT_TRUE(reply.message().empty());
    EXPECT_TRUE(reply.provider_id().empty());
    EXPECT_EQ(reply.models_size(), 0);
}

TEST(AIServiceJsonMapperTest, PopulatesKnowledgeBaseListFields)
{
    auto payload = ParseJson(R"json(
        {
          "code": 0,
          "knowledge_bases": [
            {
              "kb_id": "kb_123",
              "name": "Project Notes",
              "chunk_count": 7,
              "created_at": 1714280000,
              "status": "ready"
            }
          ]
        }
    )json");

    ai::AIKbListRsp reply;
    ai_service_json_mapper::PopulateKbListFromJson(payload, &reply);

    ASSERT_EQ(reply.code(), 0);
    ASSERT_EQ(reply.knowledge_bases_size(), 1);
    EXPECT_EQ(reply.knowledge_bases(0).kb_id(), "kb_123");
    EXPECT_EQ(reply.knowledge_bases(0).name(), "Project Notes");
    EXPECT_EQ(reply.knowledge_bases(0).chunk_count(), 7);
    EXPECT_EQ(reply.knowledge_bases(0).created_at(), 1714280000);
    EXPECT_EQ(reply.knowledge_bases(0).status(), "ready");
}

TEST(AIServiceJsonMapperTest, KnowledgeBaseListPreservesLegacyItemDefaults)
{
    auto payload = ParseJson(R"json(
        {
          "code": 0,
          "knowledge_bases": [
            {
              "kb_id": "kb_123",
              "name": 9,
              "chunk_count": "bad",
              "created_at": "bad",
              "status": false
            }
          ]
        }
    )json");

    ai::AIKbListRsp reply;
    ai_service_json_mapper::PopulateKbListFromJson(payload, &reply);

    ASSERT_EQ(reply.knowledge_bases_size(), 1);
    EXPECT_EQ(reply.knowledge_bases(0).kb_id(), "kb_123");
    EXPECT_TRUE(reply.knowledge_bases(0).name().empty());
    EXPECT_EQ(reply.knowledge_bases(0).chunk_count(), 0);
    EXPECT_EQ(reply.knowledge_bases(0).created_at(), 0);
    EXPECT_TRUE(reply.knowledge_bases(0).status().empty());
}

TEST(AIServiceJsonMapperTest, PopulatesKnowledgeBaseDeleteResponse)
{
    auto payload = ParseJson(R"json(
        {
          "code": 0,
          "message": "ok"
        }
    )json");

    ai::AIKbDeleteRsp reply;
    ai_service_json_mapper::PopulateKbDeleteFromJson(payload, &reply);

    EXPECT_EQ(reply.code(), 0);
    EXPECT_EQ(reply.message(), "ok");
}

TEST(AIServiceJsonMapperTest, KnowledgeBaseDeletePreservesLegacyMissingMessageDefault)
{
    auto payload = ParseJson(R"json({"code":0})json");

    ai::AIKbDeleteRsp reply;
    ai_service_json_mapper::PopulateKbDeleteFromJson(payload, &reply);

    EXPECT_EQ(reply.code(), 0);
    EXPECT_TRUE(reply.message().empty());
}

#include "AIServiceJsonMapper.h"

#include <gtest/gtest.h>
#include <string_view>

namespace {

memochat::json::JsonValue ParseJson(std::string_view payload) {
    memochat::json::JsonValue value;
    EXPECT_TRUE(memochat::json::reader_parse(payload, value));
    return value;
}

}  // namespace

TEST(AIServiceJsonMapperTest, PopulatesModelListAndExplicitDefaultModel) {
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

TEST(AIServiceJsonMapperTest, FallsBackToFirstModelWhenDefaultModelMissing) {
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

TEST(AIServiceJsonMapperTest, PopulatesRegisteredApiProviderModels) {
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

TEST(AIServiceJsonMapperTest, PopulatesKnowledgeBaseListFields) {
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

TEST(AIServiceJsonMapperTest, PopulatesKnowledgeBaseDeleteResponse) {
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

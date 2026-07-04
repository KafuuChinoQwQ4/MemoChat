#include "AIServiceJsonMapper.hpp"

#include "AIServiceJsonDtos.hpp"
#include "ConversationContext.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <gtest/gtest.h>

#include <array>
#include <stdexcept>
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
static_assert(memochat::reflection::FieldNamesEqual<ai_service_json_mapper::AIChatMessageJsonDto>(
    std::array<std::string_view, 4>{"msg_id", "role", "content", "created_at"}));
static_assert(memochat::reflection::FieldNamesEqual<ai_service_json_mapper::ConversationContextJsonDto>(
    std::array<std::string_view,
               7>{"session_id", "uid", "model_type", "model_name", "created_at", "last_active_at", "messages"}));
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

TEST(ConversationContextJsonTest, RoundTripsStableContextFields)
{
    ConversationContext context;
    context.session_id = "session-1";
    context.uid = 42;
    context.model_type = "ollama";
    context.model_name = "qwen3:4b";
    context.created_at = 1000;
    context.last_active_at = 2000;

    ChatMessage user_message;
    user_message.msg_id = "msg-1";
    user_message.role = "user";
    user_message.content = "hello";
    user_message.created_at = 1100;
    context.messages.push_back(user_message);

    ChatMessage assistant_message;
    assistant_message.msg_id = "msg-2";
    assistant_message.role = "assistant";
    assistant_message.content = "world";
    assistant_message.created_at = 1200;
    context.messages.push_back(assistant_message);

    const std::string body = context.ToJson();
    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::glaze_parse(root, body));
    EXPECT_EQ(root["session_id"].asString(), "session-1");
    EXPECT_EQ(root["uid"].asInt(), 42);
    const memochat::json::JsonValue messages = root["messages"];
    ASSERT_TRUE(messages.isArray());
    ASSERT_EQ(messages.size(), 2);
    EXPECT_EQ(messages[0]["role"].asString(), "user");
    EXPECT_EQ(messages[1]["content"].asString(), "world");

    const ConversationContext decoded = ConversationContext::FromJson(body);
    EXPECT_EQ(decoded.session_id, "session-1");
    EXPECT_EQ(decoded.uid, 42);
    EXPECT_EQ(decoded.model_type, "ollama");
    EXPECT_EQ(decoded.model_name, "qwen3:4b");
    EXPECT_EQ(decoded.created_at, 1000);
    EXPECT_EQ(decoded.last_active_at, 2000);
    ASSERT_EQ(decoded.messages.size(), 2);
    EXPECT_EQ(decoded.messages[0].msg_id, "msg-1");
    EXPECT_EQ(decoded.messages[1].role, "assistant");
}

TEST(ConversationContextJsonTest, FromJsonKeepsMissingAndWrongTypeDefaults)
{
    const ConversationContext decoded = ConversationContext::FromJson(R"json(
        {
          "session_id": 7,
          "uid": "bad",
          "model_type": false,
          "model_name": [],
          "created_at": "bad",
          "last_active_at": "bad",
          "messages": [
            {
              "msg_id": 9,
              "role": {},
              "content": [],
              "created_at": "bad"
            }
          ]
        }
    )json");

    EXPECT_TRUE(decoded.session_id.empty());
    EXPECT_EQ(decoded.uid, 0);
    EXPECT_TRUE(decoded.model_type.empty());
    EXPECT_TRUE(decoded.model_name.empty());
    EXPECT_EQ(decoded.created_at, 0);
    EXPECT_EQ(decoded.last_active_at, 0);
    ASSERT_EQ(decoded.messages.size(), 1);
    EXPECT_TRUE(decoded.messages[0].msg_id.empty());
    EXPECT_TRUE(decoded.messages[0].role.empty());
    EXPECT_TRUE(decoded.messages[0].content.empty());
    EXPECT_EQ(decoded.messages[0].created_at, 0);
}

TEST(ConversationContextJsonTest, FromJsonRejectsMalformedJsonAndIgnoresNonArrayMessages)
{
    EXPECT_THROW(static_cast<void>(ConversationContext::FromJson("not-json")), std::runtime_error);

    const ConversationContext decoded = ConversationContext::FromJson(R"json(
        {
          "session_id": "session-1",
          "uid": 42,
          "messages": {}
        }
    )json");

    EXPECT_EQ(decoded.session_id, "session-1");
    EXPECT_EQ(decoded.uid, 42);
    EXPECT_TRUE(decoded.messages.empty());
}

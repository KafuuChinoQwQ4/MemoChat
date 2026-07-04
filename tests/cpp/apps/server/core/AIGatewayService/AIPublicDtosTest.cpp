#include <gtest/gtest.h>

#include "services/ai/AIPublicDtos.hpp"
#include "json/GlazeCompat.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIChatRequestDto>(
    std::array<std::string_view, 6>{"uid", "session_id", "content", "model_type", "model_name", "metadata_json"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AISmartRequestDto>(
    std::array<std::string_view, 8>{"uid",
                                    "feature_type",
                                    "content",
                                    "target_lang",
                                    "context_json",
                                    "model_type",
                                    "model_name",
                                    "deployment_preference"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AISessionCreateRequestDto>(
    std::array<std::string_view, 3>{"uid", "model_type", "model_name"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIRegisterApiProviderRequestDto>(
    std::array<std::string_view, 4>{"provider_name", "base_url", "api_key", "adapter"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIKbSearchRequestDto>(
    std::array<std::string_view, 3>{"uid", "query", "top_k"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AITaskCreateRequestDto>(
    std::array<std::string_view,
               8>{"uid", "title", "content", "session_id", "model_type", "model_name", "skill_name", "metadata_json"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AISimpleResponseDto>(
    std::array<std::string_view, 2>{"code", "message"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIModelInfoResponseDto>(
    std::array<std::string_view,
               6>{"model_type", "model_name", "display_name", "is_enabled", "context_window", "supports_thinking"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIModelListResponseDto>(
    std::array<std::string_view, 3>{"code", "models", "default_model"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIRegisterApiProviderResponseDto>(
    std::array<std::string_view, 4>{"code", "message", "provider_id", "models"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIDeleteApiProviderResponseDto>(
    std::array<std::string_view, 3>{"code", "message", "provider_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIKbUploadResponseDto>(
    std::array<std::string_view, 4>{"code", "message", "chunks", "kb_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIKbSearchChunkResponseDto>(
    std::array<std::string_view, 3>{"content", "score", "source"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIKbSearchResponseDto>(
    std::array<std::string_view, 2>{"code", "chunks"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIKnowledgeBaseInfoResponseDto>(
    std::array<std::string_view, 5>{"kb_id", "name", "chunk_count", "created_at", "status"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIKnowledgeBaseListResponseDto>(
    std::array<std::string_view, 2>{"code", "knowledge_bases"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AISessionInfoResponseDto>(
    std::array<std::string_view, 6>{"session_id", "title", "model_type", "model_name", "created_at", "updated_at"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AISessionResponseDto>(
    std::array<std::string_view, 3>{"code", "message", "session"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AISessionListResponseDto>(
    std::array<std::string_view, 3>{"code", "message", "sessions"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIHistoryMessageResponseDto>(
    std::array<std::string_view, 4>{"msg_id", "role", "content", "created_at"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIHistoryResponseDto>(
    std::array<std::string_view, 2>{"code", "messages"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AISmartResponseDto>(
    std::array<std::string_view, 3>{"code", "message", "result"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIChatResponseDto>(
    std::array<std::string_view, 10>{"code",
                                     "message",
                                     "session_id",
                                     "content",
                                     "tokens",
                                     "model",
                                     "trace_id",
                                     "skill",
                                     "feedback_summary",
                                     "observations"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIMemoryItemResponseDto>(
    std::array<std::string_view, 6>{"memory_id", "type", "source", "content", "created_at", "updated_at"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::ai::AIAgentTaskItemResponseDto>(
    std::array<std::string_view, 11>{"task_id",
                                     "title",
                                     "status",
                                     "trace_id",
                                     "description",
                                     "priority",
                                     "error",
                                     "created_at",
                                     "updated_at",
                                     "completed_at",
                                     "cancelled_at"}));
#endif

namespace
{

memochat::json::JsonValue Parse(std::string_view body)
{
    memochat::json::JsonValue root;
    EXPECT_TRUE(memochat::json::glaze_parse(root, body));
    return root;
}

} // namespace

TEST(AIPublicDtosTest, DecodesChatRequestWithMetadataJsonPrecedence)
{
    memochat::gate::services::ai::AIChatRequestDto request;
    ASSERT_TRUE(memochat::gate::services::ai::DecodeAIChatRequest(
        R"({"uid":42,"session_id":"s1","content":"hello","model_type":"remote","model_name":"m1","metadata_json":"{\"a\":1}","metadata":{"ignored":true}})",
        &request));

    EXPECT_EQ(request.uid, 42);
    EXPECT_EQ(request.session_id, "s1");
    EXPECT_EQ(request.content, "hello");
    EXPECT_EQ(request.model_type, "remote");
    EXPECT_EQ(request.model_name, "m1");
    EXPECT_EQ(request.metadata_json, R"({"a":1})");
}

TEST(AIPublicDtosTest, DecodesChatMetadataObjectFallbackAndDefaults)
{
    memochat::gate::services::ai::AIChatRequestDto request;
    ASSERT_TRUE(memochat::gate::services::ai::DecodeAIChatRequest(
        R"({"uid":42,"content":"hello","metadata":{"skill":"chat","score":3}})",
        &request));

    EXPECT_EQ(request.uid, 42);
    EXPECT_TRUE(request.session_id.empty());
    EXPECT_EQ(request.content, "hello");
    EXPECT_EQ(request.model_type, "ollama");
    EXPECT_TRUE(request.model_name.empty());

    memochat::json::JsonValue metadata;
    ASSERT_TRUE(memochat::json::glaze_parse(metadata, request.metadata_json));
    EXPECT_EQ(metadata["skill"].asString(), "chat");
    EXPECT_EQ(metadata["score"].asInt(), 3);
}

TEST(AIPublicDtosTest, KeepsLegacyChatDefaultsForWrongTypes)
{
    memochat::gate::services::ai::AIChatRequestDto request;
    ASSERT_TRUE(memochat::gate::services::ai::DecodeAIChatRequest(
        R"({"uid":"bad","session_id":7,"content":false,"model_type":{},"model_name":[],"metadata_json":9})",
        &request));

    EXPECT_EQ(request.uid, 0);
    EXPECT_TRUE(request.session_id.empty());
    EXPECT_TRUE(request.content.empty());
    EXPECT_TRUE(request.model_type.empty());
    EXPECT_TRUE(request.model_name.empty());
    EXPECT_EQ(request.metadata_json, "{}");
}

TEST(AIPublicDtosTest, DecodesSmartRequestDefaults)
{
    const memochat::json::JsonValue root = Parse(R"({"uid":42,"content":"translate"})");
    const auto request = memochat::gate::services::ai::AISmartRequestFromJsonValue(root);

    EXPECT_EQ(request.uid, 42);
    EXPECT_TRUE(request.feature_type.empty());
    EXPECT_EQ(request.content, "translate");
    EXPECT_TRUE(request.target_lang.empty());
    EXPECT_EQ(request.context_json, "{}");
    EXPECT_TRUE(request.model_type.empty());
    EXPECT_TRUE(request.model_name.empty());
    EXPECT_EQ(request.deployment_preference, "any");
}

TEST(AIPublicDtosTest, DecodesProviderAndKbRequests)
{
    const auto provider = memochat::gate::services::ai::AIRegisterApiProviderRequestFromJsonValue(
        Parse(R"({"base_url":"https://api.example","api_key":"key"})"));
    EXPECT_EQ(provider.provider_name, "custom-api");
    EXPECT_EQ(provider.base_url, "https://api.example");
    EXPECT_EQ(provider.api_key, "key");
    EXPECT_EQ(provider.adapter, "openai_compatible");

    const auto kb_search =
        memochat::gate::services::ai::AIKbSearchRequestFromJsonValue(Parse(R"({"uid":42,"query":"memo","top_k":8})"));
    EXPECT_EQ(kb_search.uid, 42);
    EXPECT_EQ(kb_search.query, "memo");
    EXPECT_EQ(kb_search.top_k, 8);
}

TEST(AIPublicDtosTest, DecodesMemoryAndTaskRequests)
{
    const auto memory =
        memochat::gate::services::ai::AIMemoryDeleteRequestFromJsonValue(Parse(R"({"uid":42,"memory_id":"mem-1"})"));
    EXPECT_EQ(memory.uid, 42);
    EXPECT_EQ(memory.memory_id, "mem-1");

    memochat::gate::services::ai::AITaskCreateRequestDto task;
    ASSERT_TRUE(memochat::gate::services::ai::DecodeAITaskCreateRequest(
        R"({"uid":42,"title":"t","content":"c","session_id":"s","model_type":"remote","model_name":"m","skill_name":"planner","metadata":{"priority":"high"}})",
        &task));
    EXPECT_EQ(task.uid, 42);
    EXPECT_EQ(task.title, "t");
    EXPECT_EQ(task.content, "c");
    EXPECT_EQ(task.session_id, "s");
    EXPECT_EQ(task.model_type, "remote");
    EXPECT_EQ(task.model_name, "m");
    EXPECT_EQ(task.skill_name, "planner");

    memochat::json::JsonValue metadata;
    ASSERT_TRUE(memochat::json::glaze_parse(metadata, task.metadata_json));
    EXPECT_EQ(metadata["priority"].asString(), "high");
}

TEST(AIPublicDtosTest, DecodesTaskIdRequests)
{
    const auto task = memochat::gate::services::ai::AITaskIdRequestFromJsonValue(Parse(R"({"task_id":"task-1"})"));
    EXPECT_EQ(task.task_id, "task-1");
}

TEST(AIPublicDtosTest, RejectsMalformedJsonAndNullOutputs)
{
    memochat::gate::services::ai::AIChatRequestDto chat;
    EXPECT_FALSE(memochat::gate::services::ai::DecodeAIChatRequest("not-json", &chat));
    EXPECT_FALSE(memochat::gate::services::ai::DecodeAIChatRequest(
        R"({})",
        static_cast<memochat::gate::services::ai::AIChatRequestDto*>(nullptr)));

    memochat::gate::services::ai::AISmartRequestDto smart;
    EXPECT_FALSE(memochat::gate::services::ai::DecodeAISmartRequest("not-json", &smart));
    EXPECT_FALSE(memochat::gate::services::ai::DecodeAISmartRequest(
        R"({})",
        static_cast<memochat::gate::services::ai::AISmartRequestDto*>(nullptr)));

    memochat::gate::services::ai::AITaskCreateRequestDto task;
    EXPECT_FALSE(memochat::gate::services::ai::DecodeAITaskCreateRequest("not-json", &task));
    EXPECT_FALSE(memochat::gate::services::ai::DecodeAITaskCreateRequest(
        R"({})",
        static_cast<memochat::gate::services::ai::AITaskCreateRequestDto*>(nullptr)));
}

TEST(AIPublicDtosTest, WritesSimpleResponseWithExistingWireFields)
{
    memochat::gate::services::ai::AISimpleResponseDto response;
    response.code = 500;
    response.message = "AIServer unavailable";

    const memochat::json::JsonValue root = memochat::gate::services::ai::AISimpleResponseToJsonValue(response);

    EXPECT_EQ(root["code"].asInt(), 500);
    EXPECT_EQ(root["message"].asString(), "AIServer unavailable");
}

TEST(AIPublicDtosTest, WritesModelListResponseAndPreservesOptionalDefaultModel)
{
    memochat::gate::services::ai::AIModelInfoResponseDto model;
    model.model_type = "ollama";
    model.model_name = "llama3";
    model.display_name = "Llama 3";
    model.is_enabled = true;
    model.context_window = 8192;
    model.supports_thinking = false;

    memochat::gate::services::ai::AIModelListResponseDto response;
    response.code = 0;
    response.models.push_back(model);
    response.default_model = model;

    const memochat::json::JsonValue with_default =
        memochat::gate::services::ai::AIModelListResponseToJsonValue(response, true);
    ASSERT_TRUE(with_default.isObject()) << with_default.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(with_default, "code", -1), 0);
    const memochat::json::JsonValue models = with_default["models"];
    ASSERT_TRUE(models.isArray()) << with_default.toStyledString();
    ASSERT_EQ(models.size(), 1);
    const memochat::json::JsonValue model_json = models[0];
    ASSERT_TRUE(model_json.isObject()) << model_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(model_json, "model_type", ""), "ollama");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(model_json, "model_name", ""), "llama3");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(model_json, "display_name", ""), "Llama 3");
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(model_json, "is_enabled", false));
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(model_json, "context_window", 0), 8192);
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(model_json, "supports_thinking", true));
    ASSERT_TRUE(with_default.isMember("default_model"));
    const memochat::json::JsonValue default_model_json = with_default["default_model"];
    ASSERT_TRUE(default_model_json.isObject()) << with_default.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(default_model_json, "model_name", ""), "llama3");

    const memochat::json::JsonValue without_default =
        memochat::gate::services::ai::AIModelListResponseToJsonValue(response, false);
    ASSERT_TRUE(without_default.isObject()) << without_default.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(without_default, "code", -1), 0);
    const memochat::json::JsonValue models_without_default = without_default["models"];
    ASSERT_TRUE(models_without_default.isArray()) << without_default.toStyledString();
    EXPECT_FALSE(without_default.isMember("default_model"));
}

TEST(AIPublicDtosTest, WritesProviderResponsesWithExistingWireFields)
{
    memochat::gate::services::ai::AIModelInfoResponseDto model;
    model.model_type = "remote";
    model.model_name = "gpt";
    model.display_name = "GPT";
    model.is_enabled = true;
    model.context_window = 128000;
    model.supports_thinking = true;

    memochat::gate::services::ai::AIRegisterApiProviderResponseDto register_response;
    register_response.code = 0;
    register_response.message = "ok";
    register_response.provider_id = "provider-1";
    register_response.models.push_back(model);

    const memochat::json::JsonValue registered =
        memochat::gate::services::ai::AIRegisterApiProviderResponseToJsonValue(register_response);
    ASSERT_TRUE(registered.isObject()) << registered.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(registered, "code", -1), 0);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(registered, "message", ""), "ok");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(registered, "provider_id", ""), "provider-1");
    const memochat::json::JsonValue registered_models = registered["models"];
    ASSERT_TRUE(registered_models.isArray()) << registered.toStyledString();
    ASSERT_EQ(registered_models.size(), 1);
    const memochat::json::JsonValue registered_model = registered_models[0];
    ASSERT_TRUE(registered_model.isObject()) << registered_model.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(registered_model, "model_name", ""), "gpt");
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(registered_model, "supports_thinking", false));

    memochat::gate::services::ai::AIDeleteApiProviderResponseDto delete_response;
    delete_response.code = 0;
    delete_response.message = "deleted";
    delete_response.provider_id = "provider-1";

    const memochat::json::JsonValue deleted =
        memochat::gate::services::ai::AIDeleteApiProviderResponseToJsonValue(delete_response);
    EXPECT_EQ(deleted["code"].asInt(), 0);
    EXPECT_EQ(deleted["message"].asString(), "deleted");
    EXPECT_EQ(deleted["provider_id"].asString(), "provider-1");
}

TEST(AIPublicDtosTest, WritesKnowledgeBaseResponsesWithExistingWireFields)
{
    memochat::gate::services::ai::AIKbUploadResponseDto upload;
    upload.code = 0;
    upload.message = "ok";
    upload.chunks = 3;
    upload.kb_id = "kb-1";

    const memochat::json::JsonValue upload_json = memochat::gate::services::ai::AIKbUploadResponseToJsonValue(upload);
    EXPECT_EQ(upload_json["code"].asInt(), 0);
    EXPECT_EQ(upload_json["message"].asString(), "ok");
    EXPECT_EQ(upload_json["chunks"].asInt(), 3);
    EXPECT_EQ(upload_json["kb_id"].asString(), "kb-1");

    memochat::gate::services::ai::AIKbSearchResponseDto search;
    search.code = 0;
    search.chunks.push_back({"chunk", 0.75, "source.md"});

    const memochat::json::JsonValue search_json = memochat::gate::services::ai::AIKbSearchResponseToJsonValue(search);
    EXPECT_EQ(search_json["code"].asInt(), 0);
    ASSERT_TRUE(search_json["chunks"].isArray());
    ASSERT_EQ(search_json["chunks"].size(), 1);
    EXPECT_EQ(search_json["chunks"][0]["content"].asString(), "chunk");
    EXPECT_DOUBLE_EQ(search_json["chunks"][0]["score"].asDouble(), 0.75);
    EXPECT_EQ(search_json["chunks"][0]["source"].asString(), "source.md");

    memochat::gate::services::ai::AIKnowledgeBaseListResponseDto list;
    list.code = 0;
    list.knowledge_bases.push_back({"kb-1", "Docs", 5, 1000, "ready"});

    const memochat::json::JsonValue list_json =
        memochat::gate::services::ai::AIKnowledgeBaseListResponseToJsonValue(list);
    EXPECT_EQ(list_json["code"].asInt(), 0);
    ASSERT_TRUE(list_json["knowledge_bases"].isArray());
    ASSERT_EQ(list_json["knowledge_bases"].size(), 1);
    EXPECT_EQ(list_json["knowledge_bases"][0]["kb_id"].asString(), "kb-1");
    EXPECT_EQ(list_json["knowledge_bases"][0]["name"].asString(), "Docs");
    EXPECT_EQ(list_json["knowledge_bases"][0]["chunk_count"].asInt(), 5);
    EXPECT_EQ(list_json["knowledge_bases"][0]["created_at"].asInt64(), 1000);
    EXPECT_EQ(list_json["knowledge_bases"][0]["status"].asString(), "ready");
}

TEST(AIPublicDtosTest, WritesSessionResponseWithAndWithoutSession)
{
    memochat::gate::services::ai::AISessionResponseDto dto;
    dto.code = 0;
    dto.message = "ok";
    dto.session.session_id = "s-1";
    dto.session.title = "t";
    dto.session.model_type = "openai";
    dto.session.model_name = "gpt";
    dto.session.created_at = 100;
    dto.session.updated_at = 200;

    const auto with = memochat::gate::services::ai::AISessionResponseToJsonValue(dto, true);
    EXPECT_EQ(with["code"].asInt(), 0);
    EXPECT_EQ(with["message"].asString(), "ok");
    ASSERT_TRUE(with["session"].isObject()) << with.toStyledString();
    EXPECT_EQ(with["session"]["session_id"].asString(), "s-1");
    EXPECT_EQ(with["session"]["model_type"].asString(), "openai");

    const auto without = memochat::gate::services::ai::AISessionResponseToJsonValue(dto, false);
    EXPECT_EQ(without["code"].asInt(), 0);
    EXPECT_EQ(without["message"].asString(), "ok");
    EXPECT_FALSE(without.isMember("session")) << without.toStyledString();
}

TEST(AIPublicDtosTest, WritesSessionListResponse)
{
    memochat::gate::services::ai::AISessionListResponseDto dto;
    dto.code = 0;
    dto.message = "ok";
    memochat::gate::services::ai::AISessionInfoResponseDto s;
    s.session_id = "s-2";
    s.title = "t2";
    s.model_type = "openai";
    s.model_name = "gpt";
    s.created_at = 1;
    s.updated_at = 2;
    dto.sessions.push_back(s);

    const auto json = memochat::gate::services::ai::AISessionListResponseToJsonValue(dto);
    EXPECT_EQ(json["code"].asInt(), 0);
    EXPECT_EQ(json["message"].asString(), "ok");
    ASSERT_TRUE(json["sessions"].isArray()) << json.toStyledString();
    EXPECT_EQ(json["sessions"][0]["session_id"].asString(), "s-2");
}

TEST(AIPublicDtosTest, WritesHistoryResponse)
{
    memochat::gate::services::ai::AIHistoryResponseDto dto;
    dto.code = 0;
    memochat::gate::services::ai::AIHistoryMessageResponseDto m;
    m.msg_id = "m-1";
    m.role = "user";
    m.content = "hi";
    m.created_at = 9;
    dto.messages.push_back(m);

    const auto json = memochat::gate::services::ai::AIHistoryResponseToJsonValue(dto);
    EXPECT_EQ(json["code"].asInt(), 0);
    ASSERT_TRUE(json["messages"].isArray()) << json.toStyledString();
    EXPECT_EQ(json["messages"][0]["msg_id"].asString(), "m-1");
    EXPECT_EQ(json["messages"][0]["role"].asString(), "user");
    EXPECT_EQ(json["messages"][0]["content"].asString(), "hi");
}

TEST(AIPublicDtosTest, WritesSmartResponseWithExistingWireFields)
{
    memochat::gate::services::ai::AISmartResponseDto dto;
    dto.code = 0;
    dto.message = "ok";
    dto.result = R"({"translated":"hi"})";

    const auto json = memochat::gate::services::ai::AISmartResponseToJsonValue(dto);
    EXPECT_EQ(json["code"].asInt(), 0);
    EXPECT_EQ(json["message"].asString(), "ok");
    EXPECT_EQ(json["result"].asString(), R"({"translated":"hi"})");

    const std::string body = memochat::json::glaze_stringify(json);
    // Smart's result stays a JSON string field (not an embedded object).
    EXPECT_NE(body.find(R"("result":"{\"translated\":\"hi\"}")"), std::string::npos) << body;
}

TEST(AIPublicDtosTest, WritesChatResponseScalarShellThenEventsInWireOrder)
{
    memochat::gate::services::ai::AIChatResponseDto dto;
    dto.code = 0;
    dto.message = "ok";
    dto.session_id = "s-1";
    dto.content = "hello";
    dto.tokens = 12;
    dto.model = "llama3";
    dto.trace_id = "tr-1";
    dto.skill = "chat";
    dto.feedback_summary = "good";
    dto.observations.push_back("obs-1");
    dto.observations.push_back("obs-2");

    const auto json = memochat::gate::services::ai::AIChatResponseToJsonValue(dto, R"([{"layer":"plan"}])");
    EXPECT_EQ(json["code"].asInt(), 0);
    EXPECT_EQ(json["message"].asString(), "ok");
    EXPECT_EQ(json["session_id"].asString(), "s-1");
    EXPECT_EQ(json["content"].asString(), "hello");
    EXPECT_EQ(json["tokens"].asInt(), 12);
    EXPECT_EQ(json["model"].asString(), "llama3");
    EXPECT_EQ(json["trace_id"].asString(), "tr-1");
    EXPECT_EQ(json["skill"].asString(), "chat");
    EXPECT_EQ(json["feedback_summary"].asString(), "good");
    const memochat::json::JsonValue snapshot = json;
    ASSERT_TRUE(snapshot["observations"].isArray()) << json.toStyledString();
    ASSERT_EQ(snapshot["observations"].size(), 2U);
    EXPECT_EQ(snapshot["observations"][0].asString(), "obs-1");
    ASSERT_TRUE(snapshot["events"].isArray()) << json.toStyledString();
    ASSERT_EQ(snapshot["events"].size(), 1U);
    EXPECT_EQ(snapshot["events"][0]["layer"].asString(), "plan");

    // events is appended AFTER observations (the shell+dynamic-body wire order).
    const std::string body = memochat::json::glaze_stringify(json);
    EXPECT_LT(body.find("\"observations\""), body.find("\"events\"")) << body;
    EXPECT_LT(body.find("\"feedback_summary\""), body.find("\"observations\"")) << body;
}

TEST(AIPublicDtosTest, ChatResponseEmptyOrInvalidEventsBecomeEmptyArray)
{
    memochat::gate::services::ai::AIChatResponseDto dto;
    dto.code = 0;

    const auto empty_events = memochat::gate::services::ai::AIChatResponseToJsonValue(dto, "");
    ASSERT_TRUE(empty_events["events"].isArray()) << empty_events.toStyledString();
    EXPECT_EQ(empty_events["events"].size(), 0U);
    ASSERT_TRUE(empty_events["observations"].isArray()) << empty_events.toStyledString();
    EXPECT_EQ(empty_events["observations"].size(), 0U);

    const auto invalid_events = memochat::gate::services::ai::AIChatResponseToJsonValue(dto, "not-json");
    ASSERT_TRUE(invalid_events["events"].isArray()) << invalid_events.toStyledString();
    EXPECT_EQ(invalid_events["events"].size(), 0U);
}

TEST(AIPublicDtosTest, WritesMemoryItemScalarShellThenMetadata)
{
    memochat::gate::services::ai::AIMemoryItemResponseDto item;
    item.memory_id = "mem-1";
    item.type = "fact";
    item.source = "chat";
    item.content = "likes tea";
    item.created_at = 100;
    item.updated_at = 200;

    const auto json = memochat::gate::services::ai::AIMemoryItemResponseToJsonValue(item, R"({"weight":3})");
    EXPECT_EQ(json["memory_id"].asString(), "mem-1");
    EXPECT_EQ(json["type"].asString(), "fact");
    EXPECT_EQ(json["source"].asString(), "chat");
    EXPECT_EQ(json["content"].asString(), "likes tea");
    EXPECT_EQ(json["created_at"].asInt(), 100);
    EXPECT_EQ(json["updated_at"].asInt(), 200);
    const memochat::json::JsonValue snapshot = json;
    ASSERT_TRUE(snapshot["metadata"].isObject()) << json.toStyledString();
    EXPECT_EQ(snapshot["metadata"]["weight"].asInt(), 3);

    const std::string body = memochat::json::glaze_stringify(json);
    EXPECT_LT(body.find("\"updated_at\""), body.find("\"metadata\"")) << body;

    // Empty/invalid metadata_json -> JSON null (matches the legacy fallback).
    const auto null_meta = memochat::gate::services::ai::AIMemoryItemResponseToJsonValue(item, "");
    EXPECT_TRUE(null_meta["metadata"].isNull()) << null_meta.toStyledString();
}

TEST(AIPublicDtosTest, WritesAgentTaskItemScalarShellThenDynamicTreesInWireOrder)
{
    memochat::gate::services::ai::AIAgentTaskItemResponseDto item;
    item.task_id = "task-1";
    item.title = "Plan trip";
    item.status = "completed";
    item.trace_id = "tr-9";
    item.description = "desc";
    item.priority = 2;
    item.error = "";
    item.created_at = 1;
    item.updated_at = 2;
    item.completed_at = 3;
    item.cancelled_at = 0;

    const auto json = memochat::gate::services::ai::AIAgentTaskItemResponseToJsonValue(item,
                                                                                       R"({"step":1})",
                                                                                       R"({"ok":true})",
                                                                                       R"([{"cp":1}])",
                                                                                       R"({"src":"x"})");
    EXPECT_EQ(json["task_id"].asString(), "task-1");
    EXPECT_EQ(json["title"].asString(), "Plan trip");
    EXPECT_EQ(json["status"].asString(), "completed");
    EXPECT_EQ(json["priority"].asInt(), 2);
    EXPECT_EQ(json["completed_at"].asInt(), 3);
    const memochat::json::JsonValue snapshot = json;
    ASSERT_TRUE(snapshot["payload"].isObject()) << json.toStyledString();
    EXPECT_EQ(snapshot["payload"]["step"].asInt(), 1);
    ASSERT_TRUE(snapshot["result"].isObject()) << json.toStyledString();
    ASSERT_TRUE(snapshot["checkpoints"].isArray()) << json.toStyledString();
    EXPECT_EQ(snapshot["checkpoints"][0]["cp"].asInt(), 1);
    ASSERT_TRUE(snapshot["metadata"].isObject()) << json.toStyledString();

    // Dynamic trees appended in the existing wire order:
    // ...cancelled_at, payload, result, checkpoints, metadata.
    const std::string body = memochat::json::glaze_stringify(json);
    const auto p_payload = body.find("\"payload\"");
    const auto p_result = body.find("\"result\"");
    const auto p_checkpoints = body.find("\"checkpoints\"");
    const auto p_metadata = body.find("\"metadata\"");
    EXPECT_LT(body.find("\"cancelled_at\""), p_payload) << body;
    EXPECT_LT(p_payload, p_result) << body;
    EXPECT_LT(p_result, p_checkpoints) << body;
    EXPECT_LT(p_checkpoints, p_metadata) << body;
}

TEST(AIPublicDtosTest, AgentTaskItemEmptyDynamicJsonUsesLegacyFallbacks)
{
    memochat::gate::services::ai::AIAgentTaskItemResponseDto item;
    item.task_id = "task-2";

    const auto json = memochat::gate::services::ai::AIAgentTaskItemResponseToJsonValue(item, "", "", "", "");
    // payload/result/metadata fall back to JSON null; checkpoints to empty array.
    EXPECT_TRUE(json["payload"].isNull()) << json.toStyledString();
    EXPECT_TRUE(json["result"].isNull()) << json.toStyledString();
    EXPECT_TRUE(json["metadata"].isNull()) << json.toStyledString();
    ASSERT_TRUE(json["checkpoints"].isArray()) << json.toStyledString();
    EXPECT_EQ(json["checkpoints"].size(), 0U);
}

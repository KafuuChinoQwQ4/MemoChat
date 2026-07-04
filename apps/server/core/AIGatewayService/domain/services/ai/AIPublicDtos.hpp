#pragma once

#include "json/GlazeCompat.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace memochat::gate::services::ai
{

struct AIChatRequestDto
{
    int32_t uid = 0;
    std::string session_id;
    std::string content;
    std::string model_type = "ollama";
    std::string model_name;
    std::string metadata_json = "{}";
};

struct AISmartRequestDto
{
    int32_t uid = 0;
    std::string feature_type;
    std::string content;
    std::string target_lang;
    std::string context_json = "{}";
    std::string model_type;
    std::string model_name;
    std::string deployment_preference = "any";
};

struct AISessionCreateRequestDto
{
    int32_t uid = 0;
    std::string model_type = "ollama";
    std::string model_name;
};

struct AISessionDeleteRequestDto
{
    int32_t uid = 0;
    std::string session_id;
};

struct AISessionUpdateRequestDto
{
    int32_t uid = 0;
    std::string session_id;
    std::string title;
};

struct AIRegisterApiProviderRequestDto
{
    std::string provider_name = "custom-api";
    std::string base_url;
    std::string api_key;
    std::string adapter = "openai_compatible";
};

struct AIDeleteApiProviderRequestDto
{
    std::string provider_id;
};

struct AIKbUploadRequestDto
{
    int32_t uid = 0;
    std::string file_name;
    std::string file_type;
    std::string content;
};

struct AIKbSearchRequestDto
{
    int32_t uid = 0;
    std::string query;
    int top_k = 5;
};

struct AIKbDeleteRequestDto
{
    int32_t uid = 0;
    std::string kb_id;
};

struct AIMemoryCreateRequestDto
{
    int32_t uid = 0;
    std::string content;
};

struct AIMemoryDeleteRequestDto
{
    int32_t uid = 0;
    std::string memory_id;
};

struct AITaskCreateRequestDto
{
    int32_t uid = 0;
    std::string title;
    std::string content;
    std::string session_id;
    std::string model_type;
    std::string model_name;
    std::string skill_name;
    std::string metadata_json = "{}";
};

struct AITaskIdRequestDto
{
    std::string task_id;
};

struct AISimpleResponseDto
{
    int code = 0;
    std::string message;
};

struct AIModelInfoResponseDto
{
    std::string model_type;
    std::string model_name;
    std::string display_name;
    bool is_enabled = false;
    int64_t context_window = 0;
    bool supports_thinking = false;
};

struct AIModelListResponseDto
{
    int code = 0;
    std::vector<AIModelInfoResponseDto> models;
    AIModelInfoResponseDto default_model;
};

// Session row (mirrors ai::AISessionInfo). created_at/updated_at are double to
// preserve the existing static_cast<double> wire output.
struct AISessionInfoResponseDto
{
    std::string session_id;
    std::string title;
    std::string model_type;
    std::string model_name;
    double created_at = 0;
    double updated_at = 0;
};

// CreateSession / UpdateSession response: {code, message, session?}. The optional
// session sub-object is erased when not present (mirrors AIModelList default_model).
struct AISessionResponseDto
{
    int code = 0;
    std::string message;
    AISessionInfoResponseDto session;
};

// ListSessions response: {code, message, sessions[]}.
struct AISessionListResponseDto
{
    int code = 0;
    std::string message;
    std::vector<AISessionInfoResponseDto> sessions;
};

// GetHistory message row + response {code, messages[]}.
struct AIHistoryMessageResponseDto
{
    std::string msg_id;
    std::string role;
    std::string content;
    double created_at = 0;
};

struct AIHistoryResponseDto
{
    int code = 0;
    std::vector<AIHistoryMessageResponseDto> messages;
};

struct AIRegisterApiProviderResponseDto
{
    int code = 0;
    std::string message;
    std::string provider_id;
    std::vector<AIModelInfoResponseDto> models;
};

struct AIDeleteApiProviderResponseDto
{
    int code = 0;
    std::string message;
    std::string provider_id;
};

struct AIKbUploadResponseDto
{
    int code = 0;
    std::string message;
    int chunks = 0;
    std::string kb_id;
};

struct AIKbSearchChunkResponseDto
{
    std::string content;
    double score = 0.0;
    std::string source;
};

struct AIKbSearchResponseDto
{
    int code = 0;
    std::vector<AIKbSearchChunkResponseDto> chunks;
};

struct AIKnowledgeBaseInfoResponseDto
{
    std::string kb_id;
    std::string name;
    int chunk_count = 0;
    int64_t created_at = 0;
    std::string status;
};

struct AIKnowledgeBaseListResponseDto
{
    int code = 0;
    std::vector<AIKnowledgeBaseInfoResponseDto> knowledge_bases;
};

// Smart (non-streaming) response: {code, message, result}. result is the JSON
// string payload produced by AIServer; the success branch always carries it.
struct AISmartResponseDto
{
    int code = 0;
    std::string message;
    std::string result;
};

// Chat (non-streaming unary) response scalar shell. The dynamic `events` member
// (parsed from events_json) is appended by the caller AFTER this shell so the
// insertion order stays code..observations,events (the shell+dynamic-body
// discipline). tokens stays double to preserve the existing static_cast<double>
// wire output of tokens_used.
struct AIChatResponseDto
{
    int code = 0;
    std::string message;
    std::string session_id;
    std::string content;
    double tokens = 0;
    std::string model;
    std::string trace_id;
    std::string skill;
    std::string feedback_summary;
    std::vector<std::string> observations;
};

// Memory item scalar shell (mirrors ai::AIMemoryItem). The dynamic `metadata`
// member (parsed from metadata_json, or JSON null when absent) is appended by the
// caller AFTER this shell. created_at/updated_at stay double to preserve the
// existing static_cast<double> wire output.
struct AIMemoryItemResponseDto
{
    std::string memory_id;
    std::string type;
    std::string source;
    std::string content;
    double created_at = 0;
    double updated_at = 0;
};

// Agent task item scalar shell. Field order matches the existing hand-built wire
// order (NOT the proto field order). The dynamic `payload`/`result`/`checkpoints`/
// `metadata` members (parsed from their *_json strings) are appended by the caller
// AFTER this shell in that exact order. created_at/updated_at/completed_at/
// cancelled_at stay double to preserve the existing static_cast<double> wire output.
struct AIAgentTaskItemResponseDto
{
    std::string task_id;
    std::string title;
    std::string status;
    std::string trace_id;
    std::string description;
    int priority = 0;
    std::string error;
    double created_at = 0;
    double updated_at = 0;
    double completed_at = 0;
    double cancelled_at = 0;
};

std::string ExtractAIMetadataJson(const memochat::json::JsonValue& root);

AIChatRequestDto AIChatRequestFromJsonValue(const memochat::json::JsonValue& root);
AISmartRequestDto AISmartRequestFromJsonValue(const memochat::json::JsonValue& root);
AISessionCreateRequestDto AISessionCreateRequestFromJsonValue(const memochat::json::JsonValue& root);
AISessionDeleteRequestDto AISessionDeleteRequestFromJsonValue(const memochat::json::JsonValue& root);
AISessionUpdateRequestDto AISessionUpdateRequestFromJsonValue(const memochat::json::JsonValue& root);
AIRegisterApiProviderRequestDto AIRegisterApiProviderRequestFromJsonValue(const memochat::json::JsonValue& root);
AIDeleteApiProviderRequestDto AIDeleteApiProviderRequestFromJsonValue(const memochat::json::JsonValue& root);
AIKbUploadRequestDto AIKbUploadRequestFromJsonValue(const memochat::json::JsonValue& root);
AIKbSearchRequestDto AIKbSearchRequestFromJsonValue(const memochat::json::JsonValue& root);
AIKbDeleteRequestDto AIKbDeleteRequestFromJsonValue(const memochat::json::JsonValue& root);
AIMemoryCreateRequestDto AIMemoryCreateRequestFromJsonValue(const memochat::json::JsonValue& root);
AIMemoryDeleteRequestDto AIMemoryDeleteRequestFromJsonValue(const memochat::json::JsonValue& root);
AITaskCreateRequestDto AITaskCreateRequestFromJsonValue(const memochat::json::JsonValue& root);
AITaskIdRequestDto AITaskIdRequestFromJsonValue(const memochat::json::JsonValue& root);

bool DecodeAIChatRequest(std::string_view body, AIChatRequestDto* out, std::string* error_out = nullptr);
bool DecodeAISmartRequest(std::string_view body, AISmartRequestDto* out, std::string* error_out = nullptr);
bool DecodeAITaskCreateRequest(std::string_view body, AITaskCreateRequestDto* out, std::string* error_out = nullptr);

memochat::json::JsonValue AISimpleResponseToJsonValue(const AISimpleResponseDto& response);
memochat::json::JsonValue AIModelInfoResponseToJsonValue(const AIModelInfoResponseDto& response);
memochat::json::JsonValue AIModelListResponseToJsonValue(const AIModelListResponseDto& response);
memochat::json::JsonValue AIModelListResponseToJsonValue(const AIModelListResponseDto& response,
                                                         bool include_default_model);
memochat::json::JsonValue AISessionInfoResponseToJsonValue(const AISessionInfoResponseDto& response);
memochat::json::JsonValue AISessionResponseToJsonValue(const AISessionResponseDto& response, bool include_session);
memochat::json::JsonValue AISessionListResponseToJsonValue(const AISessionListResponseDto& response);
memochat::json::JsonValue AIHistoryResponseToJsonValue(const AIHistoryResponseDto& response);
memochat::json::JsonValue AIRegisterApiProviderResponseToJsonValue(const AIRegisterApiProviderResponseDto& response);
memochat::json::JsonValue AIDeleteApiProviderResponseToJsonValue(const AIDeleteApiProviderResponseDto& response);
memochat::json::JsonValue AIKbUploadResponseToJsonValue(const AIKbUploadResponseDto& response);
memochat::json::JsonValue AIKbSearchResponseToJsonValue(const AIKbSearchResponseDto& response);
memochat::json::JsonValue AIKnowledgeBaseListResponseToJsonValue(const AIKnowledgeBaseListResponseDto& response);

// Smart non-streaming response: pure scalar shell, no dynamic tail.
memochat::json::JsonValue AISmartResponseToJsonValue(const AISmartResponseDto& response);

// Chat non-streaming response: serializes the scalar shell, then appends the
// dynamic `events` member parsed from events_json (empty/invalid -> empty array),
// preserving the existing wire order (...observations, events).
memochat::json::JsonValue AIChatResponseToJsonValue(const AIChatResponseDto& response, const std::string& events_json);

// Memory item: serializes the scalar shell, then appends the dynamic `metadata`
// member parsed from metadata_json (empty/invalid -> JSON null).
memochat::json::JsonValue AIMemoryItemResponseToJsonValue(const AIMemoryItemResponseDto& item,
                                                          const std::string& metadata_json);

// Agent task item: serializes the scalar shell, then appends the dynamic
// `payload`/`result`/`checkpoints`/`metadata` members in that order, parsed from
// their *_json strings (empty/invalid -> JSON null, except checkpoints -> empty
// array), preserving the existing wire order.
memochat::json::JsonValue AIAgentTaskItemResponseToJsonValue(const AIAgentTaskItemResponseDto& item,
                                                             const std::string& payload_json,
                                                             const std::string& result_json,
                                                             const std::string& checkpoints_json,
                                                             const std::string& metadata_json);

} // namespace memochat::gate::services::ai

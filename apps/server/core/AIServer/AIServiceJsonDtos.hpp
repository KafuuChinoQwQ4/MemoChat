#pragma once

#include "ConversationContext.hpp"
#include "common/proto/ai_message.grpc.pb.h"
#include "json/GlazeCompat.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ai_service_json_mapper
{

struct AIModelInfoJsonDto
{
    std::string model_type;
    std::string model_name;
    std::string display_name;
    bool is_enabled = true;
    int64_t context_window = 0;
    bool supports_thinking = false;
};

struct AIModelListJsonDto
{
    int code = 0;
    std::vector<AIModelInfoJsonDto> models;
    AIModelInfoJsonDto default_model;
    bool has_default_model = false;
};

struct AIRegisterApiProviderJsonDto
{
    int code = 0;
    std::string message = "ok";
    std::string provider_id;
    std::vector<AIModelInfoJsonDto> models;
};

struct AIKnowledgeBaseInfoJsonDto
{
    std::string kb_id;
    std::string name;
    int chunk_count = 0;
    int64_t created_at = 0;
    std::string status = "ready";
};

struct AIKnowledgeBaseListJsonDto
{
    int code = 0;
    std::vector<AIKnowledgeBaseInfoJsonDto> knowledge_bases;
};

struct AIKbDeleteJsonDto
{
    int code = 0;
    std::string message = "ok";
};

struct AIChatMessageJsonDto
{
    std::string msg_id;
    std::string role;
    std::string content;
    int64_t created_at = 0;
};

struct ConversationContextJsonDto
{
    std::string session_id;
    int32_t uid = 0;
    std::string model_type;
    std::string model_name;
    int64_t created_at = 0;
    int64_t last_active_at = 0;
    std::vector<AIChatMessageJsonDto> messages;
};

AIModelInfoJsonDto AIModelInfoFromJsonValue(const memochat::json::JsonValue& model_json);
AIModelListJsonDto AIModelListFromJsonValue(const memochat::json::JsonValue& result);
AIRegisterApiProviderJsonDto AIRegisterApiProviderFromJsonValue(const memochat::json::JsonValue& result);
AIKnowledgeBaseInfoJsonDto AIKnowledgeBaseInfoFromJsonValue(const memochat::json::JsonValue& kb_json);
AIKnowledgeBaseListJsonDto AIKnowledgeBaseListFromJsonValue(const memochat::json::JsonValue& result);
AIKbDeleteJsonDto AIKbDeleteFromJsonValue(const memochat::json::JsonValue& result);

void PopulateModelInfo(const AIModelInfoJsonDto& dto, ai::ModelInfo* out);
void PopulateModelListReply(const AIModelListJsonDto& dto, ai::AIListModelsRsp* reply);
void PopulateRegisterApiProviderReply(const AIRegisterApiProviderJsonDto& dto, ai::AIRegisterApiProviderRsp* reply);
void PopulateKbListReply(const AIKnowledgeBaseListJsonDto& dto, ai::AIKbListRsp* reply);
void PopulateKbDeleteReply(const AIKbDeleteJsonDto& dto, ai::AIKbDeleteRsp* reply);

AIChatMessageJsonDto ToChatMessageJsonDto(const ChatMessage& message);
ChatMessage FromChatMessageJsonDto(const AIChatMessageJsonDto& dto);
ConversationContextJsonDto ToConversationContextJsonDto(const ConversationContext& context);
ConversationContext FromConversationContextJsonDto(const ConversationContextJsonDto& dto);
bool EncodeConversationContextJson(const ConversationContextJsonDto& dto,
                                   std::string* out,
                                   std::string* error_out = nullptr);
bool DecodeConversationContextJson(std::string_view body,
                                   ConversationContextJsonDto* out,
                                   std::string* error_out = nullptr);

} // namespace ai_service_json_mapper

#pragma once

#include "common/proto/ai_message.grpc.pb.h"
#include "json/GlazeCompat.hpp"

#include <cstdint>
#include <string>
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

} // namespace ai_service_json_mapper

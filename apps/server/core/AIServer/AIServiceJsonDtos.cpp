#include "AIServiceJsonDtos.hpp"

import memochat.ai.json_dto_algorithms;

namespace ai_service_json_mapper
{

AIModelInfoJsonDto AIModelInfoFromJsonValue(const memochat::json::JsonValue& model_json)
{
    AIModelInfoJsonDto dto;
    dto.model_type = memochat::json::glaze_safe_get<std::string>(model_json["model_type"], "");
    dto.model_name = memochat::json::glaze_safe_get<std::string>(model_json["model_name"], "");
    dto.display_name = memochat::json::glaze_safe_get<std::string>(model_json["display_name"], dto.model_name);
    dto.is_enabled = memochat::json::glaze_safe_get<bool>(model_json["is_enabled"], true);
    dto.context_window = memochat::json::glaze_safe_get<int64_t>(model_json["context_window"], 0);
    dto.supports_thinking = memochat::json::glaze_safe_get<bool>(model_json["supports_thinking"], false);
    return dto;
}

AIModelListJsonDto AIModelListFromJsonValue(const memochat::json::JsonValue& result)
{
    AIModelListJsonDto dto;
    dto.code = memochat::json::glaze_safe_get<int>(result["code"], 0);

    if (!memochat::ai::json_dto::modules::ShouldInspectObjectMembers(result.isObject()))
    {
        return dto;
    }

    const auto& object = result.impl().get<memochat::json::object_t>();
    auto models_it = object.find("models");
    const bool has_models = models_it != object.end();
    const bool models_is_array = has_models && models_it->second.holds<memochat::json::array_t>();
    if (memochat::ai::json_dto::modules::ShouldReadArrayMember(has_models, models_is_array))
    {
        const auto& models = models_it->second.get<memochat::json::array_t>();
        dto.models.reserve(models.size());
        for (const auto& model_item : models)
        {
            dto.models.push_back(AIModelInfoFromJsonValue(memochat::json::JsonValue(model_item)));
        }
    }

    auto default_model_it = object.find("default_model");
    const bool has_default_model = default_model_it != object.end();
    const bool default_model_is_object =
        has_default_model && default_model_it->second.holds<memochat::json::object_t>();
    if (memochat::ai::json_dto::modules::ShouldReadObjectMember(has_default_model, default_model_is_object))
    {
        dto.default_model = AIModelInfoFromJsonValue(memochat::json::JsonValue(default_model_it->second));
        dto.has_default_model = true;
    }
    else if (memochat::ai::json_dto::modules::ShouldFallbackToFirstModel(dto.has_default_model, dto.models.empty()))
    {
        dto.default_model = dto.models.front();
        dto.has_default_model = true;
    }

    return dto;
}

AIRegisterApiProviderJsonDto AIRegisterApiProviderFromJsonValue(const memochat::json::JsonValue& result)
{
    AIRegisterApiProviderJsonDto dto;
    dto.code = memochat::json::glaze_safe_get<int>(result["code"], 0);
    dto.message = memochat::json::glaze_safe_get<std::string>(result["message"], "ok");
    dto.provider_id = memochat::json::glaze_safe_get<std::string>(result["provider_id"], "");

    if (!memochat::ai::json_dto::modules::ShouldInspectObjectMembers(result.isObject()))
    {
        return dto;
    }

    const auto& object = result.impl().get<memochat::json::object_t>();
    auto models_it = object.find("models");
    const bool has_models = models_it != object.end();
    const bool models_is_array = has_models && models_it->second.holds<memochat::json::array_t>();
    if (!memochat::ai::json_dto::modules::ShouldReadArrayMember(has_models, models_is_array))
    {
        return dto;
    }

    const auto& models = models_it->second.get<memochat::json::array_t>();
    dto.models.reserve(models.size());
    for (const auto& model_item : models)
    {
        dto.models.push_back(AIModelInfoFromJsonValue(memochat::json::JsonValue(model_item)));
    }
    return dto;
}

AIKnowledgeBaseInfoJsonDto AIKnowledgeBaseInfoFromJsonValue(const memochat::json::JsonValue& kb_json)
{
    AIKnowledgeBaseInfoJsonDto dto;
    dto.kb_id = memochat::json::glaze_safe_get<std::string>(kb_json["kb_id"], "");
    dto.name = memochat::json::glaze_safe_get<std::string>(kb_json["name"], dto.kb_id);
    dto.chunk_count = memochat::json::glaze_safe_get<int>(kb_json["chunk_count"], 0);
    dto.created_at = memochat::json::glaze_safe_get<int64_t>(kb_json["created_at"], 0);
    dto.status = memochat::json::glaze_safe_get<std::string>(kb_json["status"], "ready");
    return dto;
}

AIKnowledgeBaseListJsonDto AIKnowledgeBaseListFromJsonValue(const memochat::json::JsonValue& result)
{
    AIKnowledgeBaseListJsonDto dto;
    dto.code = memochat::json::glaze_safe_get<int>(result["code"], 0);

    if (!memochat::ai::json_dto::modules::ShouldInspectObjectMembers(result.isObject()))
    {
        return dto;
    }

    const auto& object = result.impl().get<memochat::json::object_t>();
    auto knowledge_bases_it = object.find("knowledge_bases");
    const bool has_knowledge_bases = knowledge_bases_it != object.end();
    const bool knowledge_bases_is_array =
        has_knowledge_bases && knowledge_bases_it->second.holds<memochat::json::array_t>();
    if (!memochat::ai::json_dto::modules::ShouldReadArrayMember(has_knowledge_bases, knowledge_bases_is_array))
    {
        return dto;
    }

    const auto& knowledge_bases = knowledge_bases_it->second.get<memochat::json::array_t>();
    dto.knowledge_bases.reserve(knowledge_bases.size());
    for (const auto& kb_item : knowledge_bases)
    {
        dto.knowledge_bases.push_back(AIKnowledgeBaseInfoFromJsonValue(memochat::json::JsonValue(kb_item)));
    }
    return dto;
}

AIKbDeleteJsonDto AIKbDeleteFromJsonValue(const memochat::json::JsonValue& result)
{
    AIKbDeleteJsonDto dto;
    dto.code = memochat::json::glaze_safe_get<int>(result["code"], 0);
    dto.message = memochat::json::glaze_safe_get<std::string>(result["message"], "ok");
    return dto;
}

void PopulateModelInfo(const AIModelInfoJsonDto& dto, ai::ModelInfo* out)
{
    if (out == nullptr)
    {
        return;
    }
    out->set_model_type(dto.model_type);
    out->set_model_name(dto.model_name);
    out->set_display_name(dto.display_name);
    out->set_is_enabled(dto.is_enabled);
    out->set_context_window(dto.context_window);
    out->set_supports_thinking(dto.supports_thinking);
}

void PopulateModelListReply(const AIModelListJsonDto& dto, ai::AIListModelsRsp* reply)
{
    if (reply == nullptr)
    {
        return;
    }
    reply->clear_models();
    reply->clear_default_model();
    reply->set_code(dto.code);

    for (const auto& model_dto : dto.models)
    {
        PopulateModelInfo(model_dto, reply->add_models());
    }
    if (dto.has_default_model)
    {
        PopulateModelInfo(dto.default_model, reply->mutable_default_model());
    }
}

void PopulateRegisterApiProviderReply(const AIRegisterApiProviderJsonDto& dto, ai::AIRegisterApiProviderRsp* reply)
{
    if (reply == nullptr)
    {
        return;
    }
    reply->clear_models();
    reply->set_code(dto.code);
    reply->set_message(dto.message);
    reply->set_provider_id(dto.provider_id);

    for (const auto& model_dto : dto.models)
    {
        PopulateModelInfo(model_dto, reply->add_models());
    }
}

void PopulateKbListReply(const AIKnowledgeBaseListJsonDto& dto, ai::AIKbListRsp* reply)
{
    if (reply == nullptr)
    {
        return;
    }
    reply->clear_knowledge_bases();
    reply->set_code(dto.code);

    for (const auto& kb_dto : dto.knowledge_bases)
    {
        auto* info = reply->add_knowledge_bases();
        info->set_kb_id(kb_dto.kb_id);
        info->set_name(kb_dto.name);
        info->set_chunk_count(kb_dto.chunk_count);
        info->set_created_at(kb_dto.created_at);
        info->set_status(kb_dto.status);
    }
}

void PopulateKbDeleteReply(const AIKbDeleteJsonDto& dto, ai::AIKbDeleteRsp* reply)
{
    if (reply == nullptr)
    {
        return;
    }
    reply->set_code(dto.code);
    reply->set_message(dto.message);
}

} // namespace ai_service_json_mapper

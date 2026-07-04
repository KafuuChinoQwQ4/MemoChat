#include "AIServiceJsonDtos.hpp"

#include "json/TypedJsonCodec.hpp"

import memochat.ai.json_dto_algorithms;

#include <exception>
#include <utility>

namespace
{

bool ParseJsonForAIService(std::string_view body, memochat::json::JsonValue* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }
    if (!memochat::json::glaze_parse(*out, body))
    {
        if (error_out != nullptr)
        {
            *error_out = "invalid json";
        }
        return false;
    }
    return true;
}

template <typename T> bool WriteTypedJsonNoThrow(const T& value, std::string* out, std::string* error_out)
{
    try
    {
        return memochat::json::WriteTypedJson(value, out, error_out);
    }
    catch (const std::exception& e)
    {
        if (error_out != nullptr)
        {
            *error_out = e.what();
        }
        return false;
    }
}

} // namespace

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

AIChatMessageJsonDto ToChatMessageJsonDto(const ChatMessage& message)
{
    AIChatMessageJsonDto dto;
    dto.msg_id = message.msg_id;
    dto.role = message.role;
    dto.content = message.content;
    dto.created_at = message.created_at;
    return dto;
}

ChatMessage FromChatMessageJsonDto(const AIChatMessageJsonDto& dto)
{
    ChatMessage message;
    message.msg_id = dto.msg_id;
    message.role = dto.role;
    message.content = dto.content;
    message.created_at = dto.created_at;
    return message;
}

ConversationContextJsonDto ToConversationContextJsonDto(const ConversationContext& context)
{
    ConversationContextJsonDto dto;
    dto.session_id = context.session_id;
    dto.uid = context.uid;
    dto.model_type = context.model_type;
    dto.model_name = context.model_name;
    dto.created_at = context.created_at;
    dto.last_active_at = context.last_active_at;
    dto.messages.reserve(context.messages.size());
    for (const auto& message : context.messages)
    {
        dto.messages.push_back(ToChatMessageJsonDto(message));
    }
    return dto;
}

ConversationContext FromConversationContextJsonDto(const ConversationContextJsonDto& dto)
{
    ConversationContext context;
    context.session_id = dto.session_id;
    context.uid = dto.uid;
    context.model_type = dto.model_type;
    context.model_name = dto.model_name;
    context.created_at = dto.created_at;
    context.last_active_at = dto.last_active_at;
    context.messages.reserve(dto.messages.size());
    for (const auto& message_dto : dto.messages)
    {
        context.messages.push_back(FromChatMessageJsonDto(message_dto));
    }
    return context;
}

bool EncodeConversationContextJson(const ConversationContextJsonDto& dto, std::string* out, std::string* error_out)
{
    return WriteTypedJsonNoThrow(dto, out, error_out);
}

bool DecodeConversationContextJson(std::string_view body, ConversationContextJsonDto* out, std::string* error_out)
{
    if (out == nullptr)
    {
        if (error_out != nullptr)
        {
            *error_out = "output pointer is null";
        }
        return false;
    }

    memochat::json::JsonValue root;
    if (!ParseJsonForAIService(body, &root, error_out))
    {
        return false;
    }

    ConversationContextJsonDto dto;
    dto.session_id = memochat::json::glaze_safe_get<std::string>(root, "session_id", "");
    dto.uid = static_cast<int32_t>(memochat::json::glaze_safe_get<int64_t>(root, "uid", 0));
    dto.model_type = memochat::json::glaze_safe_get<std::string>(root, "model_type", "");
    dto.model_name = memochat::json::glaze_safe_get<std::string>(root, "model_name", "");
    dto.created_at = memochat::json::glaze_safe_get<int64_t>(root, "created_at", 0);
    dto.last_active_at = memochat::json::glaze_safe_get<int64_t>(root, "last_active_at", 0);

    if (memochat::ai::json_dto::modules::ShouldInspectObjectMembers(root.holds_object()))
    {
        const auto& obj = root.impl().get<memochat::json::object_t>();
        auto it = obj.find("messages");
        const bool has_messages = it != obj.end();
        const bool messages_is_array = has_messages && it->second.holds<memochat::json::array_t>();
        if (memochat::ai::json_dto::modules::ShouldReadArrayMember(has_messages, messages_is_array))
        {
            const auto& messages = it->second.get<memochat::json::array_t>();
            dto.messages.reserve(messages.size());
            for (const auto& message_item : messages)
            {
                memochat::json::JsonValue message_json(message_item);
                AIChatMessageJsonDto message_dto;
                message_dto.msg_id = memochat::json::glaze_safe_get<std::string>(message_json, "msg_id", "");
                message_dto.role = memochat::json::glaze_safe_get<std::string>(message_json, "role", "");
                message_dto.content = memochat::json::glaze_safe_get<std::string>(message_json, "content", "");
                message_dto.created_at = memochat::json::glaze_safe_get<int64_t>(message_json, "created_at", 0);
                dto.messages.push_back(std::move(message_dto));
            }
        }
    }

    *out = std::move(dto);
    return true;
}

} // namespace ai_service_json_mapper

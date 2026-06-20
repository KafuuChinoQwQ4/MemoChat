#include "services/ai/AIPublicDtos.h"

#include "json/TypedJsonCodec.h"

#include <exception>

namespace
{

bool ParseJsonForAIPublic(std::string_view body, memochat::json::JsonValue* out, std::string* error_out)
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

template <typename T> memochat::json::JsonValue TypedJsonToJsonValue(const T& value)
{
    std::string body;
    if (!WriteTypedJsonNoThrow(value, &body, nullptr))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }

    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, body))
    {
        return memochat::json::JsonValue(memochat::json::object_t{});
    }
    return root;
}

// Parse a dynamic JSON string the way the legacy hand-built code did: a fresh
// JsonValue, parsed only when the string is non-empty AND parses cleanly,
// otherwise the documented empty fallback. Byte-identical to the prior inline
// `reader_parse(...) ? parsed : <fallback>` logic.
memochat::json::JsonValue ParseDynamicJsonOrNull(const std::string& json_str)
{
    memochat::json::JsonValue value;
    if (!json_str.empty() && memochat::json::reader_parse(json_str, value))
    {
        return value;
    }
    return memochat::json::JsonValue{};
}

memochat::json::JsonValue ParseDynamicJsonOrEmptyArray(const std::string& json_str)
{
    memochat::json::JsonValue value;
    if (!json_str.empty() && memochat::json::reader_parse(json_str, value))
    {
        return value;
    }
    return memochat::json::JsonValue(memochat::json::array_t{});
}

} // namespace

namespace memochat::gate::services::ai
{

std::string ExtractAIMetadataJson(const memochat::json::JsonValue& root)
{
    const std::string metadata_json = memochat::json::glaze_safe_get<std::string>(root, "metadata_json", "");
    if (!metadata_json.empty())
    {
        return metadata_json;
    }
    if (memochat::json::glaze_has_key(root, "metadata"))
    {
        const memochat::json::JsonValue metadata = root["metadata"];
        return memochat::json::glaze_stringify(metadata);
    }
    return "{}";
}

AIChatRequestDto AIChatRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AIChatRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.session_id = memochat::json::glaze_safe_get<std::string>(root, "session_id", "");
    request.content = memochat::json::glaze_safe_get<std::string>(root, "content", "");
    request.model_type = memochat::json::glaze_safe_get<std::string>(root, "model_type", "ollama");
    request.model_name = memochat::json::glaze_safe_get<std::string>(root, "model_name", "");
    request.metadata_json = ExtractAIMetadataJson(root);
    return request;
}

AISmartRequestDto AISmartRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AISmartRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.feature_type = memochat::json::glaze_safe_get<std::string>(root, "feature_type", "");
    request.content = memochat::json::glaze_safe_get<std::string>(root, "content", "");
    request.target_lang = memochat::json::glaze_safe_get<std::string>(root, "target_lang", "");
    request.context_json = memochat::json::glaze_safe_get<std::string>(root, "context_json", "{}");
    request.model_type = memochat::json::glaze_safe_get<std::string>(root, "model_type", "");
    request.model_name = memochat::json::glaze_safe_get<std::string>(root, "model_name", "");
    request.deployment_preference = memochat::json::glaze_safe_get<std::string>(root, "deployment_preference", "any");
    return request;
}

AISessionCreateRequestDto AISessionCreateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AISessionCreateRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.model_type = memochat::json::glaze_safe_get<std::string>(root, "model_type", "ollama");
    request.model_name = memochat::json::glaze_safe_get<std::string>(root, "model_name", "");
    return request;
}

AISessionDeleteRequestDto AISessionDeleteRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AISessionDeleteRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.session_id = memochat::json::glaze_safe_get<std::string>(root, "session_id", "");
    return request;
}

AISessionUpdateRequestDto AISessionUpdateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AISessionUpdateRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.session_id = memochat::json::glaze_safe_get<std::string>(root, "session_id", "");
    request.title = memochat::json::glaze_safe_get<std::string>(root, "title", "");
    return request;
}

AIRegisterApiProviderRequestDto AIRegisterApiProviderRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AIRegisterApiProviderRequestDto request;
    request.provider_name = memochat::json::glaze_safe_get<std::string>(root, "provider_name", "custom-api");
    request.base_url = memochat::json::glaze_safe_get<std::string>(root, "base_url", "");
    request.api_key = memochat::json::glaze_safe_get<std::string>(root, "api_key", "");
    request.adapter = memochat::json::glaze_safe_get<std::string>(root, "adapter", "openai_compatible");
    return request;
}

AIDeleteApiProviderRequestDto AIDeleteApiProviderRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AIDeleteApiProviderRequestDto request;
    request.provider_id = memochat::json::glaze_safe_get<std::string>(root, "provider_id", "");
    return request;
}

AIKbUploadRequestDto AIKbUploadRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AIKbUploadRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.file_name = memochat::json::glaze_safe_get<std::string>(root, "file_name", "");
    request.file_type = memochat::json::glaze_safe_get<std::string>(root, "file_type", "");
    request.content = memochat::json::glaze_safe_get<std::string>(root, "content", "");
    return request;
}

AIKbSearchRequestDto AIKbSearchRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AIKbSearchRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.query = memochat::json::glaze_safe_get<std::string>(root, "query", "");
    request.top_k = memochat::json::glaze_safe_get<int>(root, "top_k", 5);
    return request;
}

AIKbDeleteRequestDto AIKbDeleteRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AIKbDeleteRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.kb_id = memochat::json::glaze_safe_get<std::string>(root, "kb_id", "");
    return request;
}

AIMemoryCreateRequestDto AIMemoryCreateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AIMemoryCreateRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.content = memochat::json::glaze_safe_get<std::string>(root, "content", "");
    return request;
}

AIMemoryDeleteRequestDto AIMemoryDeleteRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AIMemoryDeleteRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.memory_id = memochat::json::glaze_safe_get<std::string>(root, "memory_id", "");
    return request;
}

AITaskCreateRequestDto AITaskCreateRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AITaskCreateRequestDto request;
    request.uid = memochat::json::glaze_safe_get<int>(root, "uid", 0);
    request.title = memochat::json::glaze_safe_get<std::string>(root, "title", "");
    request.content = memochat::json::glaze_safe_get<std::string>(root, "content", "");
    request.session_id = memochat::json::glaze_safe_get<std::string>(root, "session_id", "");
    request.model_type = memochat::json::glaze_safe_get<std::string>(root, "model_type", "");
    request.model_name = memochat::json::glaze_safe_get<std::string>(root, "model_name", "");
    request.skill_name = memochat::json::glaze_safe_get<std::string>(root, "skill_name", "");
    request.metadata_json = ExtractAIMetadataJson(root);
    return request;
}

AITaskIdRequestDto AITaskIdRequestFromJsonValue(const memochat::json::JsonValue& root)
{
    AITaskIdRequestDto request;
    request.task_id = memochat::json::glaze_safe_get<std::string>(root, "task_id", "");
    return request;
}

bool DecodeAIChatRequest(std::string_view body, AIChatRequestDto* out, std::string* error_out)
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
    if (!ParseJsonForAIPublic(body, &root, error_out))
    {
        return false;
    }
    *out = AIChatRequestFromJsonValue(root);
    return true;
}

bool DecodeAISmartRequest(std::string_view body, AISmartRequestDto* out, std::string* error_out)
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
    if (!ParseJsonForAIPublic(body, &root, error_out))
    {
        return false;
    }
    *out = AISmartRequestFromJsonValue(root);
    return true;
}

bool DecodeAITaskCreateRequest(std::string_view body, AITaskCreateRequestDto* out, std::string* error_out)
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
    if (!ParseJsonForAIPublic(body, &root, error_out))
    {
        return false;
    }
    *out = AITaskCreateRequestFromJsonValue(root);
    return true;
}

memochat::json::JsonValue AISimpleResponseToJsonValue(const AISimpleResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIModelInfoResponseToJsonValue(const AIModelInfoResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIModelListResponseToJsonValue(const AIModelListResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIModelListResponseToJsonValue(const AIModelListResponseDto& response,
                                                         bool include_default_model)
{
    memochat::json::JsonValue root = AIModelListResponseToJsonValue(response);
    if (!include_default_model && root.isObject())
    {
        root.impl().get<memochat::json::object_t>().erase("default_model");
    }
    return root;
}

memochat::json::JsonValue AISessionInfoResponseToJsonValue(const AISessionInfoResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AISessionResponseToJsonValue(const AISessionResponseDto& response, bool include_session)
{
    memochat::json::JsonValue root = TypedJsonToJsonValue(response);
    if (!include_session && root.isObject())
    {
        root.impl().get<memochat::json::object_t>().erase("session");
    }
    return root;
}

memochat::json::JsonValue AISessionListResponseToJsonValue(const AISessionListResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIHistoryResponseToJsonValue(const AIHistoryResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIRegisterApiProviderResponseToJsonValue(
    const AIRegisterApiProviderResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIDeleteApiProviderResponseToJsonValue(
    const AIDeleteApiProviderResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIKbUploadResponseToJsonValue(const AIKbUploadResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIKbSearchResponseToJsonValue(const AIKbSearchResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIKnowledgeBaseListResponseToJsonValue(
    const AIKnowledgeBaseListResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AISmartResponseToJsonValue(const AISmartResponseDto& response)
{
    return TypedJsonToJsonValue(response);
}

memochat::json::JsonValue AIChatResponseToJsonValue(const AIChatResponseDto& response,
                                                    const std::string& events_json)
{
    memochat::json::JsonValue root = TypedJsonToJsonValue(response);
    // events is appended after the scalar shell to preserve the wire order
    // (...observations, events); empty/invalid events_json -> empty array.
    root["events"] = ParseDynamicJsonOrEmptyArray(events_json);
    return root;
}

memochat::json::JsonValue AIMemoryItemResponseToJsonValue(const AIMemoryItemResponseDto& item,
                                                          const std::string& metadata_json)
{
    memochat::json::JsonValue value = TypedJsonToJsonValue(item);
    value["metadata"] = ParseDynamicJsonOrNull(metadata_json);
    return value;
}

memochat::json::JsonValue AIAgentTaskItemResponseToJsonValue(const AIAgentTaskItemResponseDto& item,
                                                             const std::string& payload_json,
                                                             const std::string& result_json,
                                                             const std::string& checkpoints_json,
                                                             const std::string& metadata_json)
{
    memochat::json::JsonValue value = TypedJsonToJsonValue(item);
    // Dynamic members are appended in the existing wire order. checkpoints falls
    // back to an empty array; payload/result/metadata fall back to JSON null.
    value["payload"] = ParseDynamicJsonOrNull(payload_json);
    value["result"] = ParseDynamicJsonOrNull(result_json);
    value["checkpoints"] = ParseDynamicJsonOrEmptyArray(checkpoints_json);
    value["metadata"] = ParseDynamicJsonOrNull(metadata_json);
    return value;
}

} // namespace memochat::gate::services::ai

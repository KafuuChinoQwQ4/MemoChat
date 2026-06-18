#include "services/ai/AIService.h"

#include "AIServiceClient.h"
#include "json/GlazeCompat.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"

#include <cstdint>
#include <string>

namespace memochat::gate::services::ai
{
namespace
{

namespace json = memochat::json;

AIServiceClient& Client()
{
    static AIServiceClient client;
    return client;
}

void WriteJson(memochat::gate::routing::GateResponse& response, const json::JsonValue& value)
{
    response.status = 200;
    response.content_type = "application/json";
    response.body = json::glaze_stringify(value);
}

bool ParsePostJson(const memochat::gate::routing::GateRequest& request,
                   memochat::gate::routing::GateResponse& response,
                   json::JsonValue& root,
                   json::JsonValue& src_root)
{
    json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        root["error"] = 1;
        root["message"] = "invalid json";
        WriteJson(response, root);
        return false;
    }
    return true;
}

std::string ExtractMetadataJson(const json::JsonValue& src_root)
{
    const std::string metadata_json = json::glaze_safe_get<std::string>(src_root, "metadata_json", "");
    if (!metadata_json.empty())
    {
        return metadata_json;
    }
    if (json::glaze_has_key(src_root, "metadata"))
    {
        const json::JsonValue metadata = src_root["metadata"];
        return json::glaze_stringify(metadata);
    }
    return "{}";
}

void AssignQueryInt(const memochat::gate::routing::GateRequest& request, const char* key, int& value)
{
    const auto iter = request.query.find(key);
    if (iter != request.query.end())
    {
        value = std::stoi(iter->second);
    }
}

void AssignQueryInt32(const memochat::gate::routing::GateRequest& request, const char* key, int32_t& value)
{
    const auto iter = request.query.find(key);
    if (iter != request.query.end())
    {
        value = static_cast<int32_t>(std::stoi(iter->second));
    }
}

void AssignQueryString(const memochat::gate::routing::GateRequest& request, const char* key, std::string& value)
{
    const auto iter = request.query.find(key);
    if (iter != request.query.end())
    {
        value = iter->second;
    }
}

} // namespace

AIService& AIService::Instance()
{
    static AIService instance;
    return instance;
}

bool AIService::HandleChat(const memochat::gate::routing::GateRequest& request,
                           memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.chat", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string session_id = json::glaze_safe_get<std::string>(src_root, "session_id", "");
    const std::string content = json::glaze_safe_get<std::string>(src_root, "content", "");
    const std::string model_type = json::glaze_safe_get<std::string>(src_root, "model_type", "ollama");
    const std::string model_name = json::glaze_safe_get<std::string>(src_root, "model_name", "");
    const std::string metadata_json = ExtractMetadataJson(src_root);
    if (content.empty())
    {
        root["error"] = 1;
        root["message"] = "content is empty";
        WriteJson(response, root);
        return true;
    }

    memolog::LogInfo("gate.ai.chat.call", "calling AIServer Chat", {{"uid", std::to_string(uid)}});
    auto result = Client().Chat(uid, session_id, content, model_type, model_name, metadata_json);
    memolog::LogInfo(
        "gate.ai.chat.result",
        "AIServer Chat returned",
        {{"uid", std::to_string(uid)}, {"code", std::to_string(json::glaze_safe_get<int>(result, "code", 0))}});
    const int error_code = json::glaze_safe_get<int>(result, "error", json::glaze_safe_get<int>(result, "code", 0));
    if (error_code != 0)
    {
        memolog::LogError("gate.ai.chat.failed",
                          "AIServer returned error",
                          {{"uid", std::to_string(uid)}, {"code", std::to_string(error_code)}});
    }
    WriteJson(response, result);
    return true;
}

bool AIService::HandleSmart(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.smart", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string feature_type = json::glaze_safe_get<std::string>(src_root, "feature_type", "");
    const std::string content = json::glaze_safe_get<std::string>(src_root, "content", "");
    const std::string target_lang = json::glaze_safe_get<std::string>(src_root, "target_lang", "");
    const std::string context_json = json::glaze_safe_get<std::string>(src_root, "context_json", "{}");
    const std::string model_type = json::glaze_safe_get<std::string>(src_root, "model_type", "");
    const std::string model_name = json::glaze_safe_get<std::string>(src_root, "model_name", "");
    const std::string deployment_preference =
        json::glaze_safe_get<std::string>(src_root, "deployment_preference", "any");
    if (content.empty())
    {
        root["error"] = 1;
        root["message"] = "content is empty";
        WriteJson(response, root);
        return true;
    }

    auto result = Client().Smart(uid,
                                 feature_type,
                                 content,
                                 target_lang,
                                 context_json,
                                 model_type,
                                 model_name,
                                 deployment_preference);
    if (json::glaze_safe_get<int>(result, "error", json::glaze_safe_get<int>(result, "code", 0)) != 0)
    {
        memolog::LogError("gate.ai.smart.failed", "AIServer returned error", {{"feature_type", feature_type}});
    }
    WriteJson(response, result);
    return true;
}

bool AIService::HandleHistory(const memochat::gate::routing::GateRequest& request,
                              memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.history", "http");
    int32_t uid = 0;
    std::string session_id;
    int limit = 20;
    int offset = 0;
    AssignQueryInt32(request, "uid", uid);
    AssignQueryString(request, "session_id", session_id);
    AssignQueryInt(request, "limit", limit);
    AssignQueryInt(request, "offset", offset);
    auto result = Client().GetHistory(uid, session_id, limit, offset);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleCreateSession(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.session.create", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string model_type = json::glaze_safe_get<std::string>(src_root, "model_type", "ollama");
    const std::string model_name = json::glaze_safe_get<std::string>(src_root, "model_name", "");
    memolog::LogInfo("gate.ai.session.create.call", "calling AIServer CreateSession", {{"uid", std::to_string(uid)}});
    auto result = Client().CreateSession(uid, model_type, model_name);
    memolog::LogInfo(
        "gate.ai.session.create.result",
        "AIServer CreateSession returned",
        {{"uid", std::to_string(uid)}, {"code", std::to_string(json::glaze_safe_get<int>(result, "code", 0))}});
    WriteJson(response, result);
    return true;
}

bool AIService::HandleListSessions(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.session.list", "http");
    int32_t uid = 0;
    std::string model_type = "ollama";
    std::string model_name;
    AssignQueryInt32(request, "uid", uid);
    AssignQueryString(request, "model_type", model_type);
    AssignQueryString(request, "model_name", model_name);
    auto result = Client().ListSessions(uid, model_type, model_name);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleDeleteSession(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.session.delete", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string session_id = json::glaze_safe_get<std::string>(src_root, "session_id", "");
    auto result = Client().DeleteSession(uid, session_id);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleUpdateSession(const memochat::gate::routing::GateRequest& request,
                                    memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.session.update", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string session_id = json::glaze_safe_get<std::string>(src_root, "session_id", "");
    const std::string title = json::glaze_safe_get<std::string>(src_root, "title", "");
    auto result = Client().UpdateSession(uid, session_id, title);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleListModels(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    (void) request;
    memolog::SpanScope span("gate.ai.model.list", "http");
    auto result = Client().ListModels();
    WriteJson(response, result);
    return true;
}

bool AIService::HandleRegisterApiProvider(const memochat::gate::routing::GateRequest& request,
                                          memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.model.api.register", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const std::string provider_name = json::glaze_safe_get<std::string>(src_root, "provider_name", "custom-api");
    const std::string base_url = json::glaze_safe_get<std::string>(src_root, "base_url", "");
    const std::string api_key = json::glaze_safe_get<std::string>(src_root, "api_key", "");
    const std::string adapter = json::glaze_safe_get<std::string>(src_root, "adapter", "openai_compatible");
    auto result = Client().RegisterApiProvider(provider_name, base_url, api_key, adapter);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleDeleteApiProvider(const memochat::gate::routing::GateRequest& request,
                                        memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.model.api.delete", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const std::string provider_id = json::glaze_safe_get<std::string>(src_root, "provider_id", "");
    auto result = Client().DeleteApiProvider(provider_id);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleKbUpload(const memochat::gate::routing::GateRequest& request,
                               memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.kb.upload", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string file_name = json::glaze_safe_get<std::string>(src_root, "file_name", "");
    const std::string file_type = json::glaze_safe_get<std::string>(src_root, "file_type", "");
    const std::string content = json::glaze_safe_get<std::string>(src_root, "content", "");
    auto result = Client().KbUpload(uid, file_name, file_type, content);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleKbSearch(const memochat::gate::routing::GateRequest& request,
                               memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.kb.search", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string query = json::glaze_safe_get<std::string>(src_root, "query", "");
    const int top_k = json::glaze_safe_get<int>(src_root, "top_k", 5);
    auto result = Client().KbSearch(uid, query, top_k);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleListKb(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.kb.list", "http");
    int32_t uid = 0;
    AssignQueryInt32(request, "uid", uid);
    auto result = Client().ListKb(uid);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleDeleteKb(const memochat::gate::routing::GateRequest& request,
                               memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.kb.delete", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string kb_id = json::glaze_safe_get<std::string>(src_root, "kb_id", "");
    auto result = Client().DeleteKb(uid, kb_id);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleMemoryList(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.memory.list", "http");
    int32_t uid = 0;
    AssignQueryInt32(request, "uid", uid);
    auto result = Client().MemoryList(uid);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleMemoryCreate(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.memory.create", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string content = json::glaze_safe_get<std::string>(src_root, "content", "");
    auto result = Client().MemoryCreate(uid, content);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleMemoryDelete(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.memory.delete", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string memory_id = json::glaze_safe_get<std::string>(src_root, "memory_id", "");
    auto result = Client().MemoryDelete(uid, memory_id);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleTaskCreate(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.tasks.create", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
    const std::string title = json::glaze_safe_get<std::string>(src_root, "title", "");
    const std::string content = json::glaze_safe_get<std::string>(src_root, "content", "");
    const std::string session_id = json::glaze_safe_get<std::string>(src_root, "session_id", "");
    const std::string model_type = json::glaze_safe_get<std::string>(src_root, "model_type", "");
    const std::string model_name = json::glaze_safe_get<std::string>(src_root, "model_name", "");
    const std::string skill_name = json::glaze_safe_get<std::string>(src_root, "skill_name", "");
    const std::string metadata_json = ExtractMetadataJson(src_root);
    auto result =
        Client().AgentTaskCreate(uid, title, content, session_id, model_type, model_name, skill_name, metadata_json);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleTaskList(const memochat::gate::routing::GateRequest& request,
                               memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.tasks.list", "http");
    int32_t uid = 0;
    int limit = 50;
    AssignQueryInt32(request, "uid", uid);
    AssignQueryInt(request, "limit", limit);
    auto result = Client().AgentTaskList(uid, limit);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleTaskDetail(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.tasks.detail", "http");
    std::string task_id;
    AssignQueryString(request, "task_id", task_id);
    auto result = Client().AgentTaskGet(task_id);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleTaskCancel(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.tasks.cancel", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const std::string task_id = json::glaze_safe_get<std::string>(src_root, "task_id", "");
    auto result = Client().AgentTaskCancel(task_id);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleTaskResume(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.tasks.resume", "http");
    json::JsonValue root = json::JsonValue{};
    json::JsonValue src_root = json::JsonValue{};
    if (!ParsePostJson(request, response, root, src_root))
    {
        return true;
    }

    const std::string task_id = json::glaze_safe_get<std::string>(src_root, "task_id", "");
    auto result = Client().AgentTaskResume(task_id);
    WriteJson(response, result);
    return true;
}

} // namespace memochat::gate::services::ai

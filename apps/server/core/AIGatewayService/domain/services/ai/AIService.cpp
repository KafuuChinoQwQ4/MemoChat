#include "services/ai/AIService.h"

#include "AIServiceClient.h"
#include "services/ai/AIPublicDtos.h"
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

    const AIChatRequestDto chat_request = AIChatRequestFromJsonValue(src_root);
    if (chat_request.content.empty())
    {
        root["error"] = 1;
        root["message"] = "content is empty";
        WriteJson(response, root);
        return true;
    }

    memolog::LogInfo("gate.ai.chat.call", "calling AIServer Chat", {{"uid", std::to_string(chat_request.uid)}});
    auto result = Client().Chat(chat_request.uid,
                                chat_request.session_id,
                                chat_request.content,
                                chat_request.model_type,
                                chat_request.model_name,
                                chat_request.metadata_json);
    memolog::LogInfo("gate.ai.chat.result",
                     "AIServer Chat returned",
                     {{"uid", std::to_string(chat_request.uid)},
                      {"code", std::to_string(json::glaze_safe_get<int>(result, "code", 0))}});
    const int error_code = json::glaze_safe_get<int>(result, "error", json::glaze_safe_get<int>(result, "code", 0));
    if (error_code != 0)
    {
        memolog::LogError("gate.ai.chat.failed",
                          "AIServer returned error",
                          {{"uid", std::to_string(chat_request.uid)}, {"code", std::to_string(error_code)}});
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

    const AISmartRequestDto smart_request = AISmartRequestFromJsonValue(src_root);
    if (smart_request.content.empty())
    {
        root["error"] = 1;
        root["message"] = "content is empty";
        WriteJson(response, root);
        return true;
    }

    auto result = Client().Smart(smart_request.uid,
                                 smart_request.feature_type,
                                 smart_request.content,
                                 smart_request.target_lang,
                                 smart_request.context_json,
                                 smart_request.model_type,
                                 smart_request.model_name,
                                 smart_request.deployment_preference);
    if (json::glaze_safe_get<int>(result, "error", json::glaze_safe_get<int>(result, "code", 0)) != 0)
    {
        memolog::LogError("gate.ai.smart.failed",
                          "AIServer returned error",
                          {{"feature_type", smart_request.feature_type}});
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

    const AISessionCreateRequestDto session_request = AISessionCreateRequestFromJsonValue(src_root);
    memolog::LogInfo("gate.ai.session.create.call",
                     "calling AIServer CreateSession",
                     {{"uid", std::to_string(session_request.uid)}});
    auto result = Client().CreateSession(session_request.uid, session_request.model_type, session_request.model_name);
    memolog::LogInfo("gate.ai.session.create.result",
                     "AIServer CreateSession returned",
                     {{"uid", std::to_string(session_request.uid)},
                      {"code", std::to_string(json::glaze_safe_get<int>(result, "code", 0))}});
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

    const AISessionDeleteRequestDto session_request = AISessionDeleteRequestFromJsonValue(src_root);
    auto result = Client().DeleteSession(session_request.uid, session_request.session_id);
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

    const AISessionUpdateRequestDto session_request = AISessionUpdateRequestFromJsonValue(src_root);
    auto result = Client().UpdateSession(session_request.uid, session_request.session_id, session_request.title);
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

    const AIRegisterApiProviderRequestDto provider_request = AIRegisterApiProviderRequestFromJsonValue(src_root);
    auto result = Client().RegisterApiProvider(provider_request.provider_name,
                                               provider_request.base_url,
                                               provider_request.api_key,
                                               provider_request.adapter);
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

    const AIDeleteApiProviderRequestDto provider_request = AIDeleteApiProviderRequestFromJsonValue(src_root);
    auto result = Client().DeleteApiProvider(provider_request.provider_id);
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

    const AIKbUploadRequestDto kb_request = AIKbUploadRequestFromJsonValue(src_root);
    auto result = Client().KbUpload(kb_request.uid, kb_request.file_name, kb_request.file_type, kb_request.content);
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

    const AIKbSearchRequestDto kb_request = AIKbSearchRequestFromJsonValue(src_root);
    auto result = Client().KbSearch(kb_request.uid, kb_request.query, kb_request.top_k);
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

    const AIKbDeleteRequestDto kb_request = AIKbDeleteRequestFromJsonValue(src_root);
    auto result = Client().DeleteKb(kb_request.uid, kb_request.kb_id);
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

    const AIMemoryCreateRequestDto memory_request = AIMemoryCreateRequestFromJsonValue(src_root);
    auto result = Client().MemoryCreate(memory_request.uid, memory_request.content);
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

    const AIMemoryDeleteRequestDto memory_request = AIMemoryDeleteRequestFromJsonValue(src_root);
    auto result = Client().MemoryDelete(memory_request.uid, memory_request.memory_id);
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

    const AITaskCreateRequestDto task_request = AITaskCreateRequestFromJsonValue(src_root);
    auto result = Client().AgentTaskCreate(task_request.uid,
                                           task_request.title,
                                           task_request.content,
                                           task_request.session_id,
                                           task_request.model_type,
                                           task_request.model_name,
                                           task_request.skill_name,
                                           task_request.metadata_json);
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

    const AITaskIdRequestDto task_request = AITaskIdRequestFromJsonValue(src_root);
    auto result = Client().AgentTaskCancel(task_request.task_id);
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

    const AITaskIdRequestDto task_request = AITaskIdRequestFromJsonValue(src_root);
    auto result = Client().AgentTaskResume(task_request.task_id);
    WriteJson(response, result);
    return true;
}

} // namespace memochat::gate::services::ai

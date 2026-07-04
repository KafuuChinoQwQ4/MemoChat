#include "services/ai/AIService.hpp"

#include "AIServiceClient.hpp"
#include "ConfigMgr.hpp"
#include "services/ai/AIPublicDtos.hpp"
#include "json/GlazeCompat.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "support/UserTokenValidator.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <utility>

import memochat.ai.gateway_service_algorithms;

namespace memochat::gate::services::ai
{
namespace
{

namespace json = memochat::json;
namespace service_modules = memochat::gate::services::ai::service_modules;

AIServiceClient& Client()
{
    static AIServiceClient client;
    return client;
}

void WriteJson(memochat::gate::routing::GateResponse& response, const json::JsonValue& value)
{
    response.status = service_modules::SuccessStatusCode();
    response.content_type = service_modules::JsonContentType();
    response.body = json::glaze_stringify(value);
}

void WriteJsonWithStatus(memochat::gate::routing::GateResponse& response, int status, const json::JsonValue& value)
{
    response.status = status;
    response.content_type = service_modules::JsonContentType();
    response.body = json::glaze_stringify(value);
}

void WriteAuthFailure(memochat::gate::routing::GateResponse& response)
{
    json::JsonValue root = json::JsonValue{};
    root["error"] = service_modules::TokenInvalidErrorCode();
    root["code"] = service_modules::TokenInvalidErrorCode();
    root["message"] = service_modules::TokenInvalidMessage();
    WriteJsonWithStatus(response, service_modules::UnauthorizedStatusCode(), root);
}

std::string TrimCopy(std::string value)
{
    auto is_space = [](unsigned char c)
    {
        return std::isspace(c) != 0;
    };
    value.erase(value.begin(),
                std::find_if(value.begin(),
                             value.end(),
                             [&](char c)
                             {
                                 return !is_space(static_cast<unsigned char>(c));
                             }));
    value.erase(std::find_if(value.rbegin(),
                             value.rend(),
                             [&](char c)
                             {
                                 return !is_space(static_cast<unsigned char>(c));
                             })
                    .base(),
                value.end());
    return value;
}

std::string LowerAsciiCopy(std::string value)
{
    std::transform(value.begin(),
                   value.end(),
                   value.begin(),
                   [](unsigned char c)
                   {
                       return static_cast<char>(std::tolower(c));
                   });
    return value;
}

bool StartsWithCaseInsensitive(std::string_view value, std::string_view prefix)
{
    if (value.size() < prefix.size())
    {
        return false;
    }
    for (std::size_t index = 0; index < prefix.size(); ++index)
    {
        if (std::tolower(static_cast<unsigned char>(value[index])) !=
            std::tolower(static_cast<unsigned char>(prefix[index])))
        {
            return false;
        }
    }
    return true;
}

std::string HeaderValue(const memochat::gate::routing::GateRequest& request, std::string_view name)
{
    const std::string needle = LowerAsciiCopy(std::string(name));
    for (const auto& [key, value] : request.headers)
    {
        if (LowerAsciiCopy(key) == needle)
        {
            return value;
        }
    }
    return "";
}

std::string ConfigValue(std::string_view section, std::string_view key)
{
    return TrimCopy(ConfigMgr::Inst().GetValue(std::string(section), std::string(key)));
}

std::string EnvValue(const std::string& name)
{
    if (name.empty())
    {
        return "";
    }
    const char* value = std::getenv(name.c_str());
    return value == nullptr ? "" : TrimCopy(value);
}

std::string ProviderAdminAuthHeader()
{
    std::string header = ConfigValue("AIProviderAdmin", "AuthHeader");
    if (header.empty())
    {
        header = service_modules::DefaultProviderAdminAuthHeader();
    }
    return header;
}

std::string ResolveProviderAdminKey()
{
    std::string env_name = ConfigValue("AIProviderAdmin", "AdminKeyEnv");
    if (env_name.empty())
    {
        env_name = service_modules::DefaultProviderAdminKeyEnv();
    }
    if (const std::string env_value = EnvValue(env_name); !env_value.empty())
    {
        return env_value;
    }
    return ConfigValue("AIProviderAdmin", "AdminKey");
}

bool ConstantTimeEquals(std::string_view left, std::string_view right)
{
    unsigned char diff = static_cast<unsigned char>(left.size() ^ right.size());
    const std::size_t max_size = std::max(left.size(), right.size());
    for (std::size_t index = 0; index < max_size; ++index)
    {
        const unsigned char l = index < left.size() ? static_cast<unsigned char>(left[index]) : 0;
        const unsigned char r = index < right.size() ? static_cast<unsigned char>(right[index]) : 0;
        diff |= static_cast<unsigned char>(l ^ r);
    }
    return diff == 0;
}

std::string QueryValue(const memochat::gate::routing::GateRequest& request, std::string_view name)
{
    const auto iter = request.query.find(std::string(name));
    if (iter == request.query.end())
    {
        return "";
    }
    return iter->second;
}

bool TryParseInt32(std::string value, int32_t& out)
{
    value = TrimCopy(std::move(value));
    if (value.empty())
    {
        return false;
    }
    try
    {
        std::size_t parsed = 0;
        const long long raw = std::stoll(value, &parsed, 10);
        if (parsed != value.size() || raw < std::numeric_limits<int32_t>::min() ||
            raw > std::numeric_limits<int32_t>::max())
        {
            return false;
        }
        out = static_cast<int32_t>(raw);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

std::string AuthorizationToken(const std::string& authorization)
{
    std::string value = TrimCopy(authorization);
    if (value.empty())
    {
        return "";
    }
    if (StartsWithCaseInsensitive(value, "bearer "))
    {
        return TrimCopy(value.substr(7));
    }
    if (StartsWithCaseInsensitive(value, "token "))
    {
        return TrimCopy(value.substr(6));
    }
    return value;
}

std::string ExtractUserToken(const memochat::gate::routing::GateRequest& request, const json::JsonValue* body_root)
{
    for (std::string_view header_name : {"x-user-token", "x-auth-token"})
    {
        const std::string header_token = TrimCopy(HeaderValue(request, header_name));
        if (!header_token.empty())
        {
            return header_token;
        }
    }

    const std::string auth_token = AuthorizationToken(HeaderValue(request, "authorization"));
    if (!auth_token.empty())
    {
        return auth_token;
    }

    const std::string query_token = TrimCopy(QueryValue(request, "token"));
    if (!query_token.empty())
    {
        return query_token;
    }

    if (body_root != nullptr)
    {
        return TrimCopy(json::glaze_safe_get<std::string>(*body_root, "token", ""));
    }
    return "";
}

bool UseAuthUidCandidate(int32_t candidate_uid, int32_t& auth_uid)
{
    if (candidate_uid <= 0)
    {
        return false;
    }
    if (auth_uid <= 0)
    {
        auth_uid = candidate_uid;
        return true;
    }
    return auth_uid == candidate_uid;
}

bool UseAuthUidText(std::string value, int32_t& auth_uid)
{
    value = TrimCopy(std::move(value));
    if (value.empty())
    {
        return true;
    }
    int32_t uid = 0;
    return TryParseInt32(value, uid) && UseAuthUidCandidate(uid, auth_uid);
}

bool ResolveAuthenticatedUid(const memochat::gate::routing::GateRequest& request,
                             const json::JsonValue* body_root,
                             int32_t fallback_uid,
                             int32_t& auth_uid)
{
    auth_uid = 0;
    if (fallback_uid > 0 && !UseAuthUidCandidate(fallback_uid, auth_uid))
    {
        return false;
    }
    for (std::string_view header_name : {"x-user-id", "x-uid"})
    {
        if (!UseAuthUidText(HeaderValue(request, header_name), auth_uid))
        {
            return false;
        }
    }
    if (!UseAuthUidText(QueryValue(request, "uid"), auth_uid))
    {
        return false;
    }
    if (body_root != nullptr && json::glaze_has_key(*body_root, "uid"))
    {
        if (!UseAuthUidCandidate(json::glaze_safe_get<int>(*body_root, "uid", 0), auth_uid))
        {
            return false;
        }
    }
    return auth_uid > 0;
}

bool RequireUserAuth(const memochat::gate::routing::GateRequest& request,
                     memochat::gate::routing::GateResponse& response,
                     int32_t fallback_uid,
                     const json::JsonValue* body_root = nullptr,
                     int32_t* authenticated_uid = nullptr)
{
    int32_t uid = 0;
    const std::string token = ExtractUserToken(request, body_root);
    if (!ResolveAuthenticatedUid(request, body_root, fallback_uid, uid) ||
        service_modules::ShouldRejectUserAuth(uid, token.empty()) || !memochat::auth::ValidateUserToken(uid, token))
    {
        WriteAuthFailure(response);
        return false;
    }
    if (authenticated_uid != nullptr)
    {
        *authenticated_uid = uid;
    }
    return true;
}

void WriteProviderAdminFailure(memochat::gate::routing::GateResponse& response)
{
    json::JsonValue root = json::JsonValue{};
    root["error"] = service_modules::ForbiddenStatusCode();
    root["code"] = service_modules::ForbiddenStatusCode();
    root["message"] = service_modules::ProviderAdminAuthRequiredMessage();
    WriteJsonWithStatus(response, service_modules::ForbiddenStatusCode(), root);
}

bool RequireProviderAdmin(const memochat::gate::routing::GateRequest& request,
                          memochat::gate::routing::GateResponse& response)
{
    const std::string configured_key = ResolveProviderAdminKey();
    const std::string supplied = TrimCopy(HeaderValue(request, ProviderAdminAuthHeader()));
    const bool matched = ConstantTimeEquals(supplied, configured_key);
    if (service_modules::ShouldRejectProviderAdminAuth(!configured_key.empty(), supplied.empty(), matched))
    {
        WriteProviderAdminFailure(response);
        return false;
    }
    return true;
}

bool ParsePostJson(const memochat::gate::routing::GateRequest& request,
                   memochat::gate::routing::GateResponse& response,
                   json::JsonValue& root,
                   json::JsonValue& src_root)
{
    json::JsonReader reader;
    if (!reader.parse(request.body, src_root))
    {
        root["error"] = service_modules::InvalidJsonErrorCode();
        root["message"] = service_modules::InvalidJsonMessage();
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, chat_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    if (service_modules::ShouldRejectEmptyContent(chat_request.content.empty()))
    {
        root["error"] = service_modules::ContentEmptyErrorCode();
        root["message"] = service_modules::ContentEmptyMessage();
        WriteJson(response, root);
        return true;
    }

    memolog::LogInfo("gate.ai.chat.call", "calling AIServer Chat", {{"uid", std::to_string(auth_uid)}});
    auto result = Client().Chat(auth_uid,
                                chat_request.session_id,
                                chat_request.content,
                                chat_request.model_type,
                                chat_request.model_name,
                                chat_request.metadata_json);
    memolog::LogInfo(
        "gate.ai.chat.result",
        "AIServer Chat returned",
        {{"uid", std::to_string(auth_uid)}, {"code", std::to_string(json::glaze_safe_get<int>(result, "code", 0))}});
    const int error_code = json::glaze_safe_get<int>(result, "error", json::glaze_safe_get<int>(result, "code", 0));
    if (error_code != 0)
    {
        memolog::LogError("gate.ai.chat.failed",
                          "AIServer returned error",
                          {{"uid", std::to_string(auth_uid)}, {"code", std::to_string(error_code)}});
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, smart_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    if (service_modules::ShouldRejectEmptyContent(smart_request.content.empty()))
    {
        root["error"] = service_modules::ContentEmptyErrorCode();
        root["message"] = service_modules::ContentEmptyMessage();
        WriteJson(response, root);
        return true;
    }

    auto result = Client().Smart(auth_uid,
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
    int limit = service_modules::DefaultHistoryLimit();
    int offset = service_modules::DefaultHistoryOffset();
    AssignQueryInt32(request, "uid", uid);
    AssignQueryString(request, "session_id", session_id);
    AssignQueryInt(request, "limit", limit);
    AssignQueryInt(request, "offset", offset);
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, uid, nullptr, &auth_uid))
    {
        return true;
    }
    auto result = Client().GetHistory(auth_uid, session_id, limit, offset);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, session_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    memolog::LogInfo("gate.ai.session.create.call",
                     "calling AIServer CreateSession",
                     {{"uid", std::to_string(auth_uid)}});
    auto result = Client().CreateSession(auth_uid, session_request.model_type, session_request.model_name);
    memolog::LogInfo(
        "gate.ai.session.create.result",
        "AIServer CreateSession returned",
        {{"uid", std::to_string(auth_uid)}, {"code", std::to_string(json::glaze_safe_get<int>(result, "code", 0))}});
    WriteJson(response, result);
    return true;
}

bool AIService::HandleListSessions(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.session.list", "http");
    int32_t uid = 0;
    std::string model_type = service_modules::DefaultModelType();
    std::string model_name;
    AssignQueryInt32(request, "uid", uid);
    AssignQueryString(request, "model_type", model_type);
    AssignQueryString(request, "model_name", model_name);
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, uid, nullptr, &auth_uid))
    {
        return true;
    }
    auto result = Client().ListSessions(auth_uid, model_type, model_name);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, session_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().DeleteSession(auth_uid, session_request.session_id);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, session_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().UpdateSession(auth_uid, session_request.session_id, session_request.title);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleListModels(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.model.list", "http");
    if (!RequireUserAuth(request, response, 0))
    {
        return true;
    }
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
    if (!RequireUserAuth(request, response, 0, &src_root))
    {
        return true;
    }
    if (!RequireProviderAdmin(request, response))
    {
        return true;
    }
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
    if (!RequireUserAuth(request, response, 0, &src_root))
    {
        return true;
    }
    if (!RequireProviderAdmin(request, response))
    {
        return true;
    }
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, kb_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().KbUpload(auth_uid, kb_request.file_name, kb_request.file_type, kb_request.content);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, kb_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().KbSearch(auth_uid, kb_request.query, kb_request.top_k);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleListKb(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.kb.list", "http");
    int32_t uid = 0;
    AssignQueryInt32(request, "uid", uid);
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, uid, nullptr, &auth_uid))
    {
        return true;
    }
    auto result = Client().ListKb(auth_uid);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, kb_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().DeleteKb(auth_uid, kb_request.kb_id);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleMemoryList(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.memory.list", "http");
    int32_t uid = 0;
    AssignQueryInt32(request, "uid", uid);
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, uid, nullptr, &auth_uid))
    {
        return true;
    }
    auto result = Client().MemoryList(auth_uid);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, memory_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().MemoryCreate(auth_uid, memory_request.content);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, memory_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().MemoryDelete(auth_uid, memory_request.memory_id);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, task_request.uid, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().AgentTaskCreate(auth_uid,
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
    int limit = service_modules::DefaultTaskListLimit();
    AssignQueryInt32(request, "uid", uid);
    AssignQueryInt(request, "limit", limit);
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, uid, nullptr, &auth_uid))
    {
        return true;
    }
    auto result = Client().AgentTaskList(auth_uid, limit);
    WriteJson(response, result);
    return true;
}

bool AIService::HandleTaskDetail(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response)
{
    memolog::SpanScope span("gate.ai.tasks.detail", "http");
    std::string task_id;
    AssignQueryString(request, "task_id", task_id);
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, 0, nullptr, &auth_uid))
    {
        return true;
    }
    auto result = Client().AgentTaskGet(auth_uid, task_id);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, 0, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().AgentTaskCancel(auth_uid, task_request.task_id);
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
    int32_t auth_uid = 0;
    if (!RequireUserAuth(request, response, 0, &src_root, &auth_uid))
    {
        return true;
    }
    auto result = Client().AgentTaskResume(auth_uid, task_request.task_id);
    WriteJson(response, result);
    return true;
}

} // namespace memochat::gate::services::ai

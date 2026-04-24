#include "AIRouteModules.h"
#include "LogicSystem.h"
#include "HttpConnection.h"
#include "GateHttpJsonSupport.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include "AIServiceClient.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "json/GlazeCompat.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = memochat::json;

static std::unique_ptr<AIServiceClient> g_ai_client;

static void WriteJsonResponse(std::shared_ptr<HttpConnection> connection, const json::JsonValue& val) {
    connection->GetResponse().set(http::field::content_type, "application/json");
    std::string out = json::glaze_stringify(val);
    beast::ostream(connection->GetResponse().body()) << out;
}

void AIHttpServiceRoutes::RegisterRoutes(LogicSystem& logic) {
    g_ai_client = std::make_unique<AIServiceClient>();

    logic.RegPost("/ai/chat", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.chat", "http");
        json::JsonValue root = json::JsonValue{};
        json::JsonValue src_root = json::JsonValue{};
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            WriteJsonResponse(connection, root);
            return true;
        }
        int32_t uid = json::glaze_safe_get<int>(src_root["uid"], 0);
        std::string session_id = json::glaze_safe_get<std::string>(src_root["session_id"], "");
        std::string content = json::glaze_safe_get<std::string>(src_root["content"], "");
        std::string model_type = json::glaze_safe_get<std::string>(src_root["model_type"], "ollama");
        std::string model_name = json::glaze_safe_get<std::string>(src_root["model_name"], "");
        if (content.empty()) {
            root["error"] = 1;
            root["message"] = "content is empty";
            WriteJsonResponse(connection, root);
            return true;
        }
        auto result = g_ai_client->Chat(uid, session_id, content, model_type, model_name);
        if (json::glaze_safe_get<int>(result["error"], 0) != 0) {
            memolog::LogError("gate.ai.chat.failed", "AIServer returned error",
                {{"uid", std::to_string(uid)}, {"code", std::to_string(json::glaze_safe_get<int>(result["error"], 0))}});
        }
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegPost("/ai/chat/stream", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.chat.stream", "http");
        json::JsonValue root = json::JsonValue{};
        json::JsonValue src_root = json::JsonValue{};
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            connection->GetResponse().set(http::field::content_type, "text/event-stream");
            connection->GetResponse().set(http::field::cache_control, "no-cache");
            connection->GetResponse().set(http::field::connection, "keep-alive");
            beast::ostream(connection->GetResponse().body()) << "data: {\"error\":1,\"message\":\"invalid json\"}\n\n";
            return true;
        }
        int32_t uid = json::glaze_safe_get<int>(src_root["uid"], 0);
        std::string session_id = json::glaze_safe_get<std::string>(src_root["session_id"], "");
        std::string content = json::glaze_safe_get<std::string>(src_root["content"], "");
        std::string model_type = json::glaze_safe_get<std::string>(src_root["model_type"], "ollama");
        std::string model_name = json::glaze_safe_get<std::string>(src_root["model_name"], "");
        if (content.empty()) {
            connection->GetResponse().set(http::field::content_type, "text/event-stream");
            connection->GetResponse().set(http::field::cache_control, "no-cache");
            connection->GetResponse().set(http::field::connection, "keep-alive");
            beast::ostream(connection->GetResponse().body()) << "data: {\"error\":1,\"message\":\"content is empty\"}\n\n";
            return true;
        }
        connection->GetResponse().set(http::field::content_type, "text/event-stream");
        connection->GetResponse().set(http::field::cache_control, "no-cache");
        connection->GetResponse().set(http::field::connection, "keep-alive");
        g_ai_client->ChatStream(
            uid, session_id, content, model_type, model_name,
            [&connection](const std::string& chunk, bool is_final,
                         const std::string& msg_id, int64_t total_tokens) {
                json::JsonValue event = json::JsonValue{};
                event["chunk"] = chunk;
                event["is_final"] = is_final;
                event["msg_id"] = msg_id;
                event["total_tokens"] = total_tokens;
                std::string event_str = json::glaze_stringify(event);
                event_str = "data: " + event_str + "\n\n";
                beast::ostream(connection->GetResponse().body()) << event_str;
            },
            nullptr);
        return true;
    });

    logic.RegPost("/ai/smart", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.smart", "http");
        json::JsonValue root = json::JsonValue{};
        json::JsonValue src_root = json::JsonValue{};
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            WriteJsonResponse(connection, root);
            return true;
        }
        std::string feature_type = json::glaze_safe_get<std::string>(src_root["feature_type"], "");
        std::string content = json::glaze_safe_get<std::string>(src_root["content"], "");
        std::string target_lang = json::glaze_safe_get<std::string>(src_root["target_lang"], "");
        std::string context_json = json::glaze_safe_get<std::string>(src_root["context_json"], "{}");
        if (content.empty()) {
            root["error"] = 1;
            root["message"] = "content is empty";
            WriteJsonResponse(connection, root);
            return true;
        }
        auto result = g_ai_client->Smart(feature_type, content, target_lang, context_json);
        if (json::glaze_safe_get<int>(result["error"], 0) != 0) {
            memolog::LogError("gate.ai.smart.failed", "AIServer returned error", {{"feature_type", feature_type}});
        }
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegGet("/ai/history", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.history", "http");
        int32_t uid = 0;
        std::string session_id;
        int limit = 20;
        int offset = 0;
        for (const auto& param : connection->_get_params) {
            if (param.first == "uid") uid = std::stoi(param.second);
            else if (param.first == "session_id") session_id = param.second;
            else if (param.first == "limit") limit = std::stoi(param.second);
            else if (param.first == "offset") offset = std::stoi(param.second);
        }
        auto result = g_ai_client->GetHistory(uid, session_id, limit, offset);
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegPost("/ai/session", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.session.create", "http");
        json::JsonValue root = json::JsonValue{};
        json::JsonValue src_root = json::JsonValue{};
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            WriteJsonResponse(connection, root);
            return true;
        }
        int32_t uid = json::glaze_safe_get<int>(src_root["uid"], 0);
        std::string model_type = json::glaze_safe_get<std::string>(src_root["model_type"], "ollama");
        std::string model_name = json::glaze_safe_get<std::string>(src_root["model_name"], "");
        auto result = g_ai_client->CreateSession(uid, model_type, model_name);
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegGet("/ai/session/list", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.session.list", "http");
        int32_t uid = 0;
        std::string model_type = "ollama";
        std::string model_name;
        for (const auto& param : connection->_get_params) {
            if (param.first == "uid") uid = std::stoi(param.second);
            else if (param.first == "model_type") model_type = param.second;
            else if (param.first == "model_name") model_name = param.second;
        }
        auto result = g_ai_client->ListSessions(uid, model_type, model_name);
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegPost("/ai/session/delete", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.session.delete", "http");
        json::JsonValue root = json::JsonValue{};
        json::JsonValue src_root = json::JsonValue{};
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            WriteJsonResponse(connection, root);
            return true;
        }
        int32_t uid = json::glaze_safe_get<int>(src_root["uid"], 0);
        std::string session_id = json::glaze_safe_get<std::string>(src_root["session_id"], "");
        auto result = g_ai_client->DeleteSession(uid, session_id);
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegGet("/ai/model/list", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.model.list", "http");
        auto result = g_ai_client->ListModels();
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegPost("/ai/kb/upload", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.kb.upload", "http");
        json::JsonValue root = json::JsonValue{};
        json::JsonValue src_root = json::JsonValue{};
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            WriteJsonResponse(connection, root);
            return true;
        }
        int32_t uid = json::glaze_safe_get<int>(src_root["uid"], 0);
        std::string file_name = json::glaze_safe_get<std::string>(src_root["file_name"], "");
        std::string file_type = json::glaze_safe_get<std::string>(src_root["file_type"], "");
        std::string content = json::glaze_safe_get<std::string>(src_root["content"], "");
        auto result = g_ai_client->KbUpload(uid, file_name, file_type, content);
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegPost("/ai/kb/search", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.kb.search", "http");
        json::JsonValue root = json::JsonValue{};
        json::JsonValue src_root = json::JsonValue{};
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            WriteJsonResponse(connection, root);
            return true;
        }
        int32_t uid = json::glaze_safe_get<int>(src_root["uid"], 0);
        std::string query = json::glaze_safe_get<std::string>(src_root["query"], "");
        int top_k = json::glaze_safe_get<int>(src_root["top_k"], 5);
        auto result = g_ai_client->KbSearch(uid, query, top_k);
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegGet("/ai/kb/list", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.kb.list", "http");
        int32_t uid = 0;
        for (const auto& param : connection->_get_params) {
            if (param.first == "uid") uid = std::stoi(param.second);
        }
        auto result = g_ai_client->ListKb(uid);
        WriteJsonResponse(connection, result);
        return true;
    });

    logic.RegPost("/ai/kb/delete", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.kb.delete", "http");
        json::JsonValue root = json::JsonValue{};
        json::JsonValue src_root = json::JsonValue{};
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            WriteJsonResponse(connection, root);
            return true;
        }
        int32_t uid = json::glaze_safe_get<int>(src_root["uid"], 0);
        std::string kb_id = json::glaze_safe_get<std::string>(src_root["kb_id"], "");
        auto result = g_ai_client->DeleteKb(uid, kb_id);
        WriteJsonResponse(connection, result);
        return true;
    });

    memolog::LogInfo("gate.routes.registered", "AI HTTP routes registered", {{"count", "11"}});
}

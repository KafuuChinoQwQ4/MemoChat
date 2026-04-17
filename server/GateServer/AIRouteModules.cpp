#include "AIRouteModules.h"
#include "LogicSystem.h"
#include "HttpConnection.h"
#include "GateHttpJsonSupport.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include "AIServiceClient.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <json/json.h>

namespace beast = boost::beast;
namespace http = beast::http;

static std::unique_ptr<AIServiceClient> g_ai_client;

void AIHttpServiceRoutes::RegisterRoutes(LogicSystem& logic) {
    g_ai_client = std::make_unique<AIServiceClient>();

    // POST /ai/chat
    logic.RegPost("/ai/chat", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.chat", "http");

        Json::Value root;
        Json::Value src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            connection->_response.set(http::field::content_type, "application/json");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        int32_t uid = src_root.get("uid", 0).asInt();
        std::string session_id = src_root.get("session_id", "").asString();
        std::string content = src_root.get("content", "").asString();
        std::string model_type = src_root.get("model_type", "ollama").asString();
        std::string model_name = src_root.get("model_name", "").asString();

        if (content.empty()) {
            root["error"] = 1;
            root["message"] = "content is empty";
            connection->_response.set(http::field::content_type, "application/json");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        auto result = g_ai_client->Chat(uid, session_id, content, model_type, model_name);

        if (result["error"].asInt() != 0) {
            memolog::LogError("gate.ai.chat.failed",
                "AIServer returned error",
                {{"uid", std::to_string(uid)}, {"code", std::to_string(result["error"].asInt())}});
        }

        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // POST /ai/chat/stream — SSE 流式 AI 对话
    logic.RegPost("/ai/chat/stream", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.chat.stream", "http");

        Json::Value root;
        Json::Value src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            connection->_response.set(http::field::content_type, "text/event-stream");
            connection->_response.set(http::field::cache_control, "no-cache");
            connection->_response.set(http::field::connection, "keep-alive");
            beast::ostream(connection->_response.body()) << "data: {\"error\":1,\"message\":\"invalid json\"}\n\n";
            return true;
        }

        int32_t uid = src_root.get("uid", 0).asInt();
        std::string session_id = src_root.get("session_id", "").asString();
        std::string content = src_root.get("content", "").asString();
        std::string model_type = src_root.get("model_type", "ollama").asString();
        std::string model_name = src_root.get("model_name", "").asString();

        if (content.empty()) {
            connection->_response.set(http::field::content_type, "text/event-stream");
            connection->_response.set(http::field::cache_control, "no-cache");
            connection->_response.set(http::field::connection, "keep-alive");
            beast::ostream(connection->_response.body()) << "data: {\"error\":1,\"message\":\"content is empty\"}\n\n";
            return true;
        }

        connection->_response.set(http::field::content_type, "text/event-stream");
        connection->_response.set(http::field::cache_control, "no-cache");
        connection->_response.set(http::field::connection, "keep-alive");

        g_ai_client->ChatStream(
            uid, session_id, content, model_type, model_name,
            [&connection](const std::string& chunk, bool is_final,
                         const std::string& msg_id, int64_t total_tokens) {
                Json::Value event;
                event["chunk"] = chunk;
                event["is_final"] = is_final;
                event["msg_id"] = msg_id;
                event["total_tokens"] = static_cast<Json::Int64>(total_tokens);

                Json::StreamWriterBuilder builder;
                builder["indentation"] = "";
                std::string event_str = "data: " + Json::writeString(builder, event) + "\n\n";
                beast::ostream(connection->_response.body()) << event_str;
            },
            nullptr);

        return true;
    });

    // POST /ai/smart
    logic.RegPost("/ai/smart", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.smart", "http");

        Json::Value root;
        Json::Value src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            connection->_response.set(http::field::content_type, "application/json");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        std::string feature_type = src_root.get("feature_type", "").asString();
        std::string content = src_root.get("content", "").asString();
        std::string target_lang = src_root.get("target_lang", "").asString();
        std::string context_json = src_root.get("context_json", "{}").asString();

        if (content.empty()) {
            root["error"] = 1;
            root["message"] = "content is empty";
            connection->_response.set(http::field::content_type, "application/json");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        auto result = g_ai_client->Smart(feature_type, content, target_lang, context_json);

        if (result["error"].asInt() != 0) {
            memolog::LogError("gate.ai.smart.failed",
                "AIServer returned error",
                {{"feature_type", feature_type}});
        }

        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // GET /ai/history
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
        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // POST /ai/session
    logic.RegPost("/ai/session", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.session.create", "http");

        Json::Value root;
        Json::Value src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            connection->_response.set(http::field::content_type, "application/json");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        int32_t uid = src_root.get("uid", 0).asInt();
        std::string model_type = src_root.get("model_type", "ollama").asString();
        std::string model_name = src_root.get("model_name", "").asString();

        auto result = g_ai_client->CreateSession(uid, model_type, model_name);
        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // GET /ai/session/list
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
        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // POST /ai/session/delete
    logic.RegPost("/ai/session/delete", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.session.delete", "http");

        Json::Value root;
        Json::Value src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            connection->_response.set(http::field::content_type, "application/json");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        int32_t uid = src_root.get("uid", 0).asInt();
        std::string session_id = src_root.get("session_id", "").asString();

        auto result = g_ai_client->DeleteSession(uid, session_id);
        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // GET /ai/model/list
    logic.RegGet("/ai/model/list", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.model.list", "http");

        auto result = g_ai_client->ListModels();
        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // POST /ai/kb/upload
    logic.RegPost("/ai/kb/upload", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.kb.upload", "http");

        Json::Value root;
        Json::Value src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            connection->_response.set(http::field::content_type, "application/json");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        int32_t uid = src_root.get("uid", 0).asInt();
        std::string file_name = src_root.get("file_name", "").asString();
        std::string file_type = src_root.get("file_type", "").asString();
        std::string content = src_root.get("content", "").asString();

        auto result = g_ai_client->KbUpload(uid, file_name, file_type, content);
        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // POST /ai/kb/search
    logic.RegPost("/ai/kb/search", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.kb.search", "http");

        Json::Value root;
        Json::Value src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            connection->_response.set(http::field::content_type, "application/json");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        int32_t uid = src_root.get("uid", 0).asInt();
        std::string query = src_root.get("query", "").asString();
        int top_k = src_root.get("top_k", 5).asInt();

        auto result = g_ai_client->KbSearch(uid, query, top_k);
        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // GET /ai/kb/list
    logic.RegGet("/ai/kb/list", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.kb.list", "http");

        int32_t uid = 0;
        for (const auto& param : connection->_get_params) {
            if (param.first == "uid") uid = std::stoi(param.second);
        }

        auto result = g_ai_client->ListKb(uid);
        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    // POST /ai/kb/delete
    logic.RegPost("/ai/kb/delete", [](std::shared_ptr<HttpConnection> connection) {
        memolog::SpanScope span("gate.ai.kb.delete", "http");

        Json::Value root;
        Json::Value src_root;
        if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root)) {
            root["error"] = 1;
            root["message"] = "invalid json";
            connection->_response.set(http::field::content_type, "application/json");
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        int32_t uid = src_root.get("uid", 0).asInt();
        std::string kb_id = src_root.get("kb_id", "").asString();

        auto result = g_ai_client->DeleteKb(uid, kb_id);
        connection->_response.set(http::field::content_type, "application/json");
        beast::ostream(connection->_response.body()) << result.toStyledString();
        return true;
    });

    memolog::LogInfo("gate.routes.registered", "AI HTTP routes registered", {{"count", "11"}});
}

#include "AIRouteModules.hpp"
#include "LogicSystem.hpp"
#include "HttpConnection.hpp"
#include "GateHttpJsonSupport.hpp"
#include "GateWorkerPool.hpp"
#include "logging/Logger.hpp"
#include "logging/TraceContext.hpp"
#include "AIServiceClient.hpp"
#include "ConfigMgr.hpp"
#include "support/UserTokenValidator.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <string_view>
#include <utility>
#include "json/GlazeCompat.hpp"

import memochat.ai.route_module_algorithms;
import memochat.ai.gateway_service_algorithms;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace json = memochat::json;

static std::unique_ptr<AIServiceClient> g_ai_stream_client;

static void WriteJsonResponse(std::shared_ptr<HttpConnection> connection, const json::JsonValue& val)
{
    connection->GetResponse().set(http::field::content_type, "application/json");
    std::string out = json::glaze_stringify(val);
    beast::ostream(connection->GetResponse().body()) << out;
}

static std::string ExtractMetadataJson(const json::JsonValue& src_root)
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

static std::string AiOrchestratorHost()
{
    auto& cfg = ConfigMgr::Inst();
    std::string host = cfg["AIOrchestrator"]["Host"];
    return host.empty() ? "127.0.0.1" : host;
}

static std::string AiOrchestratorPort()
{
    auto& cfg = ConfigMgr::Inst();
    std::string port = cfg["AIOrchestrator"]["Port"];
    return port.empty() ? "8096" : port;
}

static int AiOrchestratorTimeoutSec()
{
    auto& cfg = ConfigMgr::Inst();
    std::string raw = cfg["AIOrchestrator"]["TimeoutSec"];
    if (raw.empty())
    {
        return 300;
    }
    try
    {
        return memochat::gate::modules::ai::route_algorithms::AtLeast(std::stoi(raw), 1);
    }
    catch (...)
    {
        return 300;
    }
}

static std::string TrimCopy(std::string value)
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

static std::string LowerAsciiCopy(std::string value)
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

static bool StartsWithCaseInsensitive(std::string_view value, std::string_view prefix)
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

static std::string HeaderValue(std::shared_ptr<HttpConnection> connection, std::string_view name)
{
    if (!connection)
    {
        return "";
    }
    const std::string needle = LowerAsciiCopy(std::string(name));
    for (const auto& field : connection->GetRequest())
    {
        if (LowerAsciiCopy(std::string(field.name_string())) == needle)
        {
            return std::string(field.value());
        }
    }
    return "";
}

static std::string QueryValue(std::shared_ptr<HttpConnection> connection, std::string_view name)
{
    if (!connection)
    {
        return "";
    }
    const auto& query = connection->GetParams();
    const auto iter = query.find(std::string(name));
    if (iter == query.end())
    {
        return "";
    }
    return iter->second;
}

static bool TryParseInt32(std::string value, int32_t& out)
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

static std::string AuthorizationToken(const std::string& authorization)
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

static std::string ExtractUserToken(std::shared_ptr<HttpConnection> connection, const json::JsonValue* body_root)
{
    for (std::string_view header_name : {"x-user-token", "x-auth-token"})
    {
        const std::string header_token = TrimCopy(HeaderValue(connection, header_name));
        if (!header_token.empty())
        {
            return header_token;
        }
    }

    const std::string auth_token = AuthorizationToken(HeaderValue(connection, "authorization"));
    if (!auth_token.empty())
    {
        return auth_token;
    }

    const std::string query_token = TrimCopy(QueryValue(connection, "token"));
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

static bool UseAuthUidCandidate(int32_t candidate_uid, int32_t& auth_uid)
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

static bool UseAuthUidText(std::string value, int32_t& auth_uid)
{
    value = TrimCopy(std::move(value));
    if (value.empty())
    {
        return true;
    }
    int32_t uid = 0;
    return TryParseInt32(value, uid) && UseAuthUidCandidate(uid, auth_uid);
}

static bool ResolveAuthenticatedUid(std::shared_ptr<HttpConnection> connection,
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
        if (!UseAuthUidText(HeaderValue(connection, header_name), auth_uid))
        {
            return false;
        }
    }
    if (!UseAuthUidText(QueryValue(connection, "uid"), auth_uid))
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

static void WriteAuthFailure(std::shared_ptr<HttpConnection> connection)
{
    if (!connection)
    {
        return;
    }
    json::JsonValue root = json::JsonValue{};
    root["error"] = memochat::gate::services::ai::service_modules::TokenInvalidErrorCode();
    root["code"] = memochat::gate::services::ai::service_modules::TokenInvalidErrorCode();
    root["message"] = memochat::gate::services::ai::service_modules::TokenInvalidMessage();
    connection->GetResponse().result(
        static_cast<http::status>(memochat::gate::services::ai::service_modules::UnauthorizedStatusCode()));
    WriteJsonResponse(connection, root);
}

static bool RequireConnectionUserAuth(std::shared_ptr<HttpConnection> connection,
                                      const json::JsonValue* body_root,
                                      int32_t fallback_uid = 0,
                                      int32_t* authenticated_uid = nullptr)
{
    int32_t uid = 0;
    const std::string token = ExtractUserToken(connection, body_root);
    if (!ResolveAuthenticatedUid(connection, body_root, fallback_uid, uid) ||
        memochat::gate::services::ai::service_modules::ShouldRejectUserAuth(uid, token.empty()) ||
        !memochat::auth::ValidateUserToken(uid, token))
    {
        WriteAuthFailure(connection);
        return false;
    }
    if (authenticated_uid != nullptr)
    {
        *authenticated_uid = uid;
    }
    return true;
}

static const json::JsonValue* ParseOptionalBodyRoot(const std::string& body, json::JsonValue& body_root)
{
    if (body.empty())
    {
        return nullptr;
    }
    if (!json::reader_parse(body, body_root))
    {
        return nullptr;
    }
    return &body_root;
}

static std::string ConfigValue(std::string_view section, std::string_view key)
{
    auto& cfg = ConfigMgr::Inst();
    return cfg.GetValue(std::string(section), std::string(key));
}

static std::string EnvValue(const std::string& name)
{
    if (name.empty())
    {
        return "";
    }
    const char* value = std::getenv(name.c_str());
    return value == nullptr ? std::string() : std::string(value);
}

static std::string AiOrchestratorInternalAuthHeader()
{
    std::string header = TrimCopy(ConfigValue("AIOrchestrator", "InternalAuthHeader"));
    return header.empty() ? "X-MemoChat-AI-Internal-Key" : header;
}

static std::string AiOrchestratorInternalApiKey()
{
    std::string key = TrimCopy(ConfigValue("AIOrchestrator", "InternalApiKey"));
    if (!key.empty())
    {
        return key;
    }
    std::string env_name = TrimCopy(ConfigValue("AIOrchestrator", "InternalApiKeyEnv"));
    if (env_name.empty())
    {
        env_name = "MEMOCHAT_AI_INTERNAL_API_KEY";
    }
    return TrimCopy(EnvValue(env_name));
}

template <typename Body> static void MaybeSetAiOrchestratorInternalAuth(http::request<Body>& req)
{
    const std::string api_key = AiOrchestratorInternalApiKey();
    if (!api_key.empty())
    {
        req.set(AiOrchestratorInternalAuthHeader(), api_key);
    }
}

static std::string IncomingBodyString(std::shared_ptr<HttpConnection> connection)
{
    return connection ? connection->RequestBodyString() : std::string();
}

static std::string BuildPrefixProxyTarget(std::shared_ptr<HttpConnection> connection,
                                          const std::string& gate_prefix,
                                          const std::string& orchestrator_prefix)
{
    const std::string target = connection ? connection->RequestTargetString() : std::string();
    if (!memochat::gate::modules::ai::route_algorithms::StartsWith(target.data(),
                                                                   target.size(),
                                                                   gate_prefix.data(),
                                                                   gate_prefix.size()))
    {
        return orchestrator_prefix;
    }
    return orchestrator_prefix + target.substr(gate_prefix.size());
}

static void ProxyAiOrchestratorPrefix(std::shared_ptr<HttpConnection> connection,
                                      http::verb verb,
                                      const std::string& gate_prefix,
                                      const std::string& orchestrator_prefix,
                                      const std::string& log_name)
{
    json::JsonValue auth_body_root;
    const json::JsonValue* auth_body =
        ParseOptionalBodyRoot(verb == http::verb::get ? std::string() : IncomingBodyString(connection), auth_body_root);
    if (!RequireConnectionUserAuth(connection, auth_body))
    {
        return;
    }

    const std::string host = AiOrchestratorHost();
    const std::string port = AiOrchestratorPort();
    const int timeout_sec = AiOrchestratorTimeoutSec();
    const std::string target = BuildPrefixProxyTarget(connection, gate_prefix, orchestrator_prefix);

    try
    {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        stream.expires_after(std::chrono::seconds(timeout_sec));
        stream.connect(resolver.resolve(host, port));

        http::request<http::string_body> req{verb, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::accept, "*/*");
        req.set("X-Trace-Id", connection ? connection->GetTraceId() : "");
        req.set("X-Request-Id", connection ? connection->GetRequestId() : "");
        MaybeSetAiOrchestratorInternalAuth(req);
        if (verb == http::verb::post)
        {
            req.set(http::field::content_type, "application/json");
            req.body() = IncomingBodyString(connection);
        }
        req.prepare_payload();

        http::write(stream, req);
        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        stream.expires_after(std::chrono::seconds(timeout_sec));
        http::read(stream, buffer, res);
        beast::error_code shutdown_ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, shutdown_ec);

        connection->GetResponse().result(res.result());
        const auto content_type_it = res.base().find(http::field::content_type);
        const std::string upstream_content_type =
            content_type_it != res.base().end()
                ? std::string(content_type_it->value().data(), content_type_it->value().size())
                : std::string();
        connection->GetResponse().set(http::field::content_type,
                                      upstream_content_type.empty() ? "application/json; charset=utf-8"
                                                                    : upstream_content_type);
        beast::ostream(connection->GetResponse().body()) << res.body();
        memolog::LogInfo(log_name + ".ok",
                         "AI prefix proxy returned",
                         {
                             {"target", target},
                             {"status", std::to_string(res.result_int())},
                         });
    }
    catch (const std::exception& exc)
    {
        json::JsonValue root = json::JsonValue{};
        root["code"] = 503;
        root["message"] = std::string("AIOrchestrator proxy failed: ") + exc.what();
        connection->GetResponse().result(http::status::service_unavailable);
        WriteJsonResponse(connection, root);
        memolog::LogError(log_name + ".failed",
                          "AI prefix proxy failed",
                          {
                              {"target", target},
                              {"error", exc.what()},
                          });
    }
}

static void ProxyAiOrchestratorGame(std::shared_ptr<HttpConnection> connection, http::verb verb)
{
    ProxyAiOrchestratorPrefix(connection, verb, "/ai/games", "/agent/games", "gate.ai.games.proxy");
}

static void ProxyAiOrchestratorPet(std::shared_ptr<HttpConnection> connection, http::verb verb)
{
    ProxyAiOrchestratorPrefix(connection, verb, "/ai/pet", "/pet", "gate.ai.pet.proxy");
}

static void ProxyAiOrchestratorPetStream(std::shared_ptr<HttpConnection> connection)
{
    const std::string host = AiOrchestratorHost();
    const std::string port = AiOrchestratorPort();
    const int timeout_sec = AiOrchestratorTimeoutSec();
    const std::string target = BuildPrefixProxyTarget(connection, "/ai/pet", "/pet");

    try
    {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        stream.expires_after(std::chrono::seconds(timeout_sec));
        stream.connect(resolver.resolve(host, port));

        http::request<http::string_body> req{http::verb::get, target, 11};
        req.set(http::field::host, host);
        req.set(http::field::accept, "text/event-stream");
        req.set(http::field::cache_control, "no-cache");
        req.set("X-Trace-Id", connection ? connection->GetTraceId() : "");
        req.set("X-Request-Id", connection ? connection->GetRequestId() : "");
        MaybeSetAiOrchestratorInternalAuth(req);
        req.prepare_payload();

        http::write(stream, req);

        beast::flat_buffer buffer;
        http::response_parser<http::dynamic_body> parser;
        parser.body_limit((std::numeric_limits<std::uint64_t>::max)());
        stream.expires_after(std::chrono::seconds(timeout_sec));
        http::read_header(stream, buffer, parser);

        auto forward_body = [&]()
        {
            auto& body = parser.get().body();
            if (!connection || body.size() == 0)
            {
                return;
            }
            std::string chunk = beast::buffers_to_string(body.data());
            body.consume(body.size());
            if (!chunk.empty())
            {
                connection->WriteStreamChunk(std::move(chunk));
            }
        };

        if (parser.get().result() != http::status::ok)
        {
            json::JsonValue event = json::JsonValue{};
            event["type"] = "pet.error";
            event["message"] = "AIOrchestrator returned " + std::to_string(parser.get().result_int());
            if (connection)
            {
                connection->WriteStreamChunk("data: " + json::glaze_stringify(event) + "\n\n");
            }
            forward_body();
        }
        else
        {
            forward_body();
            while (!parser.is_done())
            {
                beast::error_code ec;
                stream.expires_after(std::chrono::seconds(timeout_sec));
                http::read_some(stream, buffer, parser, ec);
                forward_body();
                if (!ec)
                {
                    continue;
                }
                if (ec == http::error::need_buffer)
                {
                    continue;
                }
                if (ec == net::error::eof)
                {
                    break;
                }
                memolog::LogWarn("gate.ai.pet.proxy.stream.read_failed",
                                 "AI pet stream read failed",
                                 {
                                     {"target", target},
                                     {"error", ec.message()},
                                 });
                break;
            }
        }

        beast::error_code shutdown_ec;
        stream.socket().shutdown(tcp::socket::shutdown_both, shutdown_ec);
        memolog::LogInfo("gate.ai.pet.proxy.stream.ok",
                         "AI pet stream proxy finished",
                         {
                             {"target", target},
                         });
    }
    catch (const std::exception& exc)
    {
        json::JsonValue event = json::JsonValue{};
        event["type"] = "pet.error";
        event["message"] = std::string("AIOrchestrator pet stream failed: ") + exc.what();
        if (connection)
        {
            connection->WriteStreamChunk("data: " + json::glaze_stringify(event) + "\n\n");
        }
        memolog::LogError("gate.ai.pet.proxy.stream.failed",
                          "AI pet stream proxy failed",
                          {
                              {"target", target},
                              {"error", exc.what()},
                          });
    }

    if (connection)
    {
        connection->FinishStream();
    }
}

void AIHttpServiceRoutes::RegisterRoutes(LogicSystem& logic)
{
    g_ai_stream_client = std::make_unique<AIServiceClient>();

    logic.RegGetPrefix("/ai/games",
                       [](std::shared_ptr<HttpConnection> connection)
                       {
                           memolog::SpanScope span("gate.ai.games.get", "http");
                           ProxyAiOrchestratorGame(connection, http::verb::get);
                           return true;
                       });

    logic.RegPostPrefix("/ai/games",
                        [](std::shared_ptr<HttpConnection> connection)
                        {
                            memolog::SpanScope span("gate.ai.games.post", "http");
                            ProxyAiOrchestratorGame(connection, http::verb::post);
                            return true;
                        });

    logic.RegDeletePrefix("/ai/games",
                          [](std::shared_ptr<HttpConnection> connection)
                          {
                              memolog::SpanScope span("gate.ai.games.delete", "http");
                              ProxyAiOrchestratorGame(connection, http::verb::delete_);
                              return true;
                          });

    logic.RegGetPrefix("/ai/pet",
                       [](std::shared_ptr<HttpConnection> connection)
                       {
                           memolog::SpanScope span("gate.ai.pet.get", "http");
                           const std::string target = connection ? connection->RequestTargetString() : std::string();
                           if (target.find("/stream") != std::string::npos)
                           {
                               if (!RequireConnectionUserAuth(connection, nullptr))
                               {
                                   return true;
                               }
                               if (connection)
                               {
                                   connection->StartSseStream();
                               }
                               GateWorkerPool::GetInstance()->post(
                                   [connection]()
                                   {
                                       ProxyAiOrchestratorPetStream(connection);
                                   });
                               return true;
                           }
                           ProxyAiOrchestratorPet(connection, http::verb::get);
                           return true;
                       });

    logic.RegPostPrefix("/ai/pet",
                        [](std::shared_ptr<HttpConnection> connection)
                        {
                            memolog::SpanScope span("gate.ai.pet.post", "http");
                            ProxyAiOrchestratorPet(connection, http::verb::post);
                            return true;
                        });

    logic.RegPost("/ai/chat/stream",
                  [](std::shared_ptr<HttpConnection> connection)
                  {
                      memolog::SpanScope span("gate.ai.chat.stream", "http");
                      json::JsonValue root = json::JsonValue{};
                      json::JsonValue src_root = json::JsonValue{};
                      if (!GateHttpJsonSupport::ParseJsonBody(connection, root, src_root))
                      {
                          connection->GetResponse().set(http::field::content_type, "text/event-stream");
                          connection->GetResponse().set(http::field::cache_control, "no-cache");
                          connection->GetResponse().set(http::field::connection, "keep-alive");
                          beast::ostream(connection->GetResponse().body())
                              << "data: {\"error\":1,\"message\":\"invalid json\"}\n\n";
                          return true;
                      }
                      const int32_t request_uid = json::glaze_safe_get<int>(src_root, "uid", 0);
                      std::string session_id = json::glaze_safe_get<std::string>(src_root, "session_id", "");
                      std::string content = json::glaze_safe_get<std::string>(src_root, "content", "");
                      std::string model_type = json::glaze_safe_get<std::string>(src_root, "model_type", "ollama");
                      std::string model_name = json::glaze_safe_get<std::string>(src_root, "model_name", "");
                      std::string metadata_json = ExtractMetadataJson(src_root);
                      int32_t auth_uid = 0;
                      if (!RequireConnectionUserAuth(connection, &src_root, request_uid, &auth_uid))
                      {
                          return true;
                      }
                      if (content.empty())
                      {
                          connection->GetResponse().set(http::field::content_type, "text/event-stream");
                          connection->GetResponse().set(http::field::cache_control, "no-cache");
                          connection->GetResponse().set(http::field::connection, "keep-alive");
                          beast::ostream(connection->GetResponse().body())
                              << "data: {\"error\":1,\"message\":\"content is empty\"}\n\n";
                          return true;
                      }
                      connection->StartSseStream();
                      memolog::LogInfo("gate.ai.chat_stream.call",
                                       "calling AIServer ChatStream",
                                       {{"uid", std::to_string(auth_uid)}});
                      try
                      {
                          g_ai_stream_client->ChatStream(
                              auth_uid,
                              session_id,
                              content,
                              model_type,
                              model_name,
                              metadata_json,
                              [&connection](const std::string& chunk,
                                            bool is_final,
                                            const std::string& msg_id,
                                            int64_t total_tokens,
                                            const std::string& trace_id,
                                            const std::string& skill,
                                            const std::string& feedback_summary,
                                            const std::string& observations_json,
                                            const std::string& events_json)
                              {
                                  json::JsonValue event = json::JsonValue{};
                                  event["chunk"] = chunk;
                                  event["is_final"] = is_final;
                                  event["msg_id"] = msg_id;
                                  event["total_tokens"] = total_tokens;
                                  event["trace_id"] = trace_id;
                                  event["skill"] = skill;
                                  event["feedback_summary"] = feedback_summary;
                                  json::JsonValue observations;
                                  if (!observations_json.empty() && json::reader_parse(observations_json, observations))
                                  {
                                      event["observations"] = observations;
                                  }
                                  else
                                  {
                                      event["observations"] = json::array_t{};
                                  }
                                  json::JsonValue events;
                                  if (!events_json.empty() && json::reader_parse(events_json, events))
                                  {
                                      event["events"] = events;
                                  }
                                  else
                                  {
                                      event["events"] = json::array_t{};
                                  }
                                  std::string event_str = json::glaze_stringify(event);
                                  event_str = "data: " + event_str + "\n\n";
                                  connection->WriteStreamChunk(std::move(event_str));
                              },
                              nullptr);
                      }
                      catch (const std::exception& e)
                      {
                          memolog::LogError("gate.ai.chat_stream.exception",
                                            "AIServer ChatStream threw",
                                            {{"uid", std::to_string(auth_uid)}, {"error", e.what()}});
                          json::JsonValue event = json::JsonValue{};
                          event["chunk"] = std::string("AIServer unavailable: ") + e.what();
                          event["is_final"] = true;
                          event["msg_id"] = "";
                          event["total_tokens"] = 0;
                          event["trace_id"] = "";
                          event["skill"] = "";
                          event["feedback_summary"] = "";
                          event["observations"] = json::array_t{};
                          event["events"] = json::array_t{};
                          connection->WriteStreamChunk("data: " + json::glaze_stringify(event) + "\n\n");
                      }
                      connection->FinishStream();
                      memolog::LogInfo("gate.ai.chat_stream.result",
                                       "AIServer ChatStream returned",
                                       {{"uid", std::to_string(auth_uid)}});
                      return true;
                  });

    memolog::LogInfo("gate.routes.registered", "AI HTTP legacy routes registered", {{"count", "6"}});
}

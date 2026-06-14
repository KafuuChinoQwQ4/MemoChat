#include "AIRouteModules.h"
#include "LogicSystem.h"
#include "HttpConnection.h"
#include "GateHttpJsonSupport.h"
#include "GateWorkerPool.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include "AIServiceClient.h"
#include "ConfigMgr.h"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <algorithm>
#include <chrono>
#include <limits>
#include "json/GlazeCompat.h"

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
        return std::max(1, std::stoi(raw));
    }
    catch (...)
    {
        return 300;
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
    if (target.rfind(gate_prefix, 0) != 0)
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

    logic.RegPost(
        "/ai/chat/stream",
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
            int32_t uid = json::glaze_safe_get<int>(src_root, "uid", 0);
            std::string session_id = json::glaze_safe_get<std::string>(src_root, "session_id", "");
            std::string content = json::glaze_safe_get<std::string>(src_root, "content", "");
            std::string model_type = json::glaze_safe_get<std::string>(src_root, "model_type", "ollama");
            std::string model_name = json::glaze_safe_get<std::string>(src_root, "model_name", "");
            std::string metadata_json = ExtractMetadataJson(src_root);
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
            memolog::LogInfo("gate.ai.chat_stream.call", "calling AIServer ChatStream", {{"uid", std::to_string(uid)}});
            try
            {
                g_ai_stream_client->ChatStream(
                    uid,
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
                                  {{"uid", std::to_string(uid)}, {"error", e.what()}});
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
                             {{"uid", std::to_string(uid)}});
            return true;
        });

    memolog::LogInfo("gate.routes.registered", "AI HTTP legacy routes registered", {{"count", "6"}});
}

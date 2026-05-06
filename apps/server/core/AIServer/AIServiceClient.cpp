#include "AIServiceClient.h"
#include "ConfigMgr.h"
#include "logging/Logger.h"
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <sstream>
#include <thread>
#include <future>
#include <chrono>
#include <limits>
#include <cctype>
#include <iomanip>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace json = memochat::json;

class AIServiceClient::Impl {
public:
    Impl() {
        auto& cfg = ConfigMgr::Inst();
        _host = cfg["AIOrchestrator"]["Host"];
        _port = cfg["AIOrchestrator"]["Port"];
        _timeout_sec = std::stoi(cfg["AIOrchestrator"]["TimeoutSec"].empty() ? "300" : cfg["AIOrchestrator"]["TimeoutSec"]);
    }

    static std::string UrlEncode(const std::string& input) {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex << std::uppercase;
        for (unsigned char ch : input) {
            if (std::isalnum(ch) || ch == '-' || ch == '_' || ch == '.' || ch == '~') {
                escaped << static_cast<char>(ch);
            } else {
                escaped << '%' << std::setw(2) << static_cast<int>(ch);
            }
        }
        return escaped.str();
    }

    grpc::Status RequestJson(http::verb verb,
                             const std::string& path,
                             const std::string& body_str,
                             bool has_body,
                             json::JsonValue* out_result) {
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);
            stream.expires_after(std::chrono::seconds(_timeout_sec));

            auto const results = resolver.resolve(_host, _port);
            stream.connect(results);

            http::request<http::string_body> req{verb, path, 11};
            req.set(http::field::host, _host);
            if (has_body) {
                req.set(http::field::content_type, "application/json");
                req.body() = body_str;
            }
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            stream.expires_after(std::chrono::seconds(_timeout_sec));
            http::read(stream, buffer, res);

            if (res.result() != http::status::ok) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                   "AIOrchestrator returned " + std::to_string(res.result_int()));
            }

            bool ok = memochat::json::reader_parse(res.body(), *out_result);
            if (!ok) {
                return grpc::Status(grpc::StatusCode::INTERNAL, "JSON parse error");
            }

            stream.socket().close();
            return grpc::Status::OK;
        } catch (const std::exception& e) {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, e.what());
        }
    }

    grpc::Status PostJson(const std::string& path,
                          const json::JsonValue& body,
                          json::JsonValue* out_result) {
        std::string body_str = memochat::json::glaze_stringify(body);
        return RequestJson(http::verb::post, path, body_str, true, out_result);
    }

    grpc::Status GetJson(const std::string& path,
                         json::JsonValue* out_result) {
        return RequestJson(http::verb::get, path, "", false, out_result);
    }

    grpc::Status DeleteJson(const std::string& path,
                            json::JsonValue* out_result) {
        return RequestJson(http::verb::delete_, path, "", false, out_result);
    }

    static void AttachMetadataJson(json::JsonValue& body, const std::string& metadata_json) {
        if (metadata_json.empty()) {
            return;
        }
        json::JsonValue metadata = json::JsonValue{};
        if (memochat::json::reader_parse(metadata_json, metadata) && metadata.isObject()) {
            body["metadata"] = std::move(metadata);
        }
    }

    grpc::Status PostJsonSSE(const std::string& path,
                              const json::JsonValue& body,
                              ChunkCallback on_chunk,
                              json::JsonValue* out_result) {
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);
            stream.expires_after(std::chrono::seconds(_timeout_sec));

            auto const results = resolver.resolve(_host, _port);
            stream.connect(results);

            std::string body_str = memochat::json::glaze_stringify(body);

            http::request<http::string_body> req{http::verb::post, path, 11};
            req.set(http::field::host, _host);
            req.set(http::field::content_type, "application/json");
            req.body() = body_str;
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response_parser<http::dynamic_body> parser;
            parser.body_limit((std::numeric_limits<std::uint64_t>::max)());
            stream.expires_after(std::chrono::seconds(_timeout_sec));

            std::string accumulated;
            std::string current_msg_id;
            int64_t total_tokens = 0;
            bool saw_final = false;

            auto parse_and_emit = [&](const std::string& line) -> bool {
                if (line.size() < 6 || line.substr(0, 6) != "data: ") return false;
                std::string json_str = line.substr(6);
                if (json_str == "[DONE]") {
                    saw_final = true;
                    return true;
                }

                json::JsonValue chunk_obj = json::JsonValue{};
                bool ok = memochat::json::reader_parse(json_str, chunk_obj);
                if (!ok) return false;

                std::string chunk_text = json::glaze_safe_get<std::string>(chunk_obj["chunk"], "");
                bool final_flag = json::glaze_safe_get<bool>(chunk_obj["is_final"], false);
                std::string msg_id = json::glaze_safe_get<std::string>(chunk_obj["msg_id"], "");
                std::string trace_id = json::glaze_safe_get<std::string>(chunk_obj["trace_id"], "");
                std::string skill = json::glaze_safe_get<std::string>(chunk_obj["skill"], "");
                std::string feedback_summary = json::glaze_safe_get<std::string>(chunk_obj["feedback_summary"], "");
                std::string observations_json = "[]";
                std::string events_json = "[]";
                if (memochat::json::glaze_has_key(chunk_obj, "observations")) {
                    observations_json = memochat::json::glaze_stringify(chunk_obj["observations"].get<json::JsonValue>());
                }
                if (memochat::json::glaze_has_key(chunk_obj, "events")) {
                    events_json = memochat::json::glaze_stringify(chunk_obj["events"].get<json::JsonValue>());
                }
                if (!msg_id.empty()) {
                    current_msg_id = msg_id;
                }
                int64_t tokens = json::glaze_safe_get<int64_t>(chunk_obj, "total_tokens", 0);
                if (tokens > 0) total_tokens = tokens;

                accumulated += chunk_text;
                if (on_chunk) {
                    on_chunk(
                        chunk_text,
                        final_flag,
                        current_msg_id,
                        total_tokens,
                        trace_id,
                        skill,
                        feedback_summary,
                        observations_json,
                        events_json);
                }

                if (final_flag && out_result) {
                    (*out_result)["code"] = 0;
                    (*out_result)["content"] = accumulated;
                    (*out_result)["tokens"] = total_tokens;
                    (*out_result)["trace_id"] = trace_id;
                    (*out_result)["skill"] = skill;
                    (*out_result)["feedback_summary"] = feedback_summary;
                }
                if (final_flag) {
                    saw_final = true;
                }
                return final_flag;
            };

            auto trim_line = [](std::string& line) {
                const auto end = line.find_last_not_of(" \t\r");
                if (end == std::string::npos) {
                    line.clear();
                } else {
                    line.erase(end + 1);
                }
            };

            std::string pending;
            auto consume_body = [&]() -> bool {
                auto& body = parser.get().body();
                if (body.size() > 0) {
                    pending += beast::buffers_to_string(body.data());
                    body.consume(body.size());
                }

                for (;;) {
                    const auto eol = pending.find('\n');
                    if (eol == std::string::npos) {
                        return false;
                    }
                    std::string line = pending.substr(0, eol);
                    pending.erase(0, eol + 1);
                    trim_line(line);
                    if (!line.empty() && parse_and_emit(line)) {
                        return true;
                    }
                }
            };

            http::read_header(stream, buffer, parser);
            if (parser.get().result() != http::status::ok) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                   "AIOrchestrator returned " + std::to_string(parser.get().result_int()));
            }

            bool done = consume_body();
            while (!done && !parser.is_done()) {
                stream.expires_after(std::chrono::seconds(_timeout_sec));
                http::read_some(stream, buffer, parser);
                done = consume_body();
            }
            if (!done && !pending.empty()) {
                trim_line(pending);
                if (!pending.empty()) {
                    done = parse_and_emit(pending);
                }
                pending.clear();
            }
            if (!saw_final && out_result) {
                (*out_result)["code"] = 0;
                (*out_result)["content"] = accumulated;
                (*out_result)["tokens"] = total_tokens;
            }

            stream.socket().close();
            return grpc::Status::OK;
        } catch (const std::exception& e) {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, e.what());
        }
    }

    std::string _host;
    std::string _port;
    int _timeout_sec = 120;
};

AIServiceClient::AIServiceClient()
    : _impl(std::make_unique<Impl>()) {}

AIServiceClient::~AIServiceClient() = default;

grpc::Status AIServiceClient::Chat(int32_t uid,
                                    const std::string& session_id,
                                    const std::string& content,
                                    const std::string& model_type,
                                    const std::string& model_name,
                                    const std::string& metadata_json,
                                    json::JsonValue* out_result) {
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["session_id"] = session_id;
    body["content"] = content;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["stream"] = false;
    Impl::AttachMetadataJson(body, metadata_json);

    return _impl->PostJson("/chat", body, out_result);
}

grpc::Status AIServiceClient::ChatStream(int32_t uid,
                                          const std::string& session_id,
                                          const std::string& content,
                                          const std::string& model_type,
                                          const std::string& model_name,
                                          const std::string& metadata_json,
                                          ChunkCallback on_chunk,
                                          json::JsonValue* out_result) {
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["session_id"] = session_id;
    body["content"] = content;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["stream"] = true;
    Impl::AttachMetadataJson(body, metadata_json);

    return _impl->PostJsonSSE("/chat/stream", body, on_chunk, out_result);
}

grpc::Status AIServiceClient::Smart(int32_t uid,
                                     const std::string& feature_type,
                                     const std::string& content,
                                     const std::string& target_lang,
                                     const std::string& context_json,
                                     const std::string& model_type,
                                     const std::string& model_name,
                                     const std::string& deployment_preference,
                                     json::JsonValue* out_result) {
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["feature_type"] = feature_type;
    body["content"] = content;
    body["target_lang"] = target_lang;
    body["context_json"] = context_json;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["deployment_preference"] = deployment_preference.empty() ? "any" : deployment_preference;

    return _impl->PostJson("/smart", body, out_result);
}

grpc::Status AIServiceClient::KbUpload(int32_t uid,
                                        const std::string& file_name,
                                        const std::string& file_type,
                                        const std::string& base64_content,
                                        json::JsonValue* out_result) {
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["file_name"] = file_name;
    body["file_type"] = file_type;
    body["content"] = base64_content;

    return _impl->PostJson("/kb/upload", body, out_result);
}

grpc::Status AIServiceClient::KbSearch(int32_t uid,
                                        const std::string& query,
                                        int top_k,
                                        json::JsonValue* out_result) {
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["query"] = query;
    body["top_k"] = top_k;

    return _impl->PostJson("/kb/search", body, out_result);
}

grpc::Status AIServiceClient::ListModels(json::JsonValue* out_result) {
    return _impl->GetJson("/models", out_result);
}

grpc::Status AIServiceClient::RegisterApiProvider(const std::string& provider_name,
                                                   const std::string& base_url,
                                                   const std::string& api_key,
                                                   const std::string& adapter,
                                                   json::JsonValue* out_result) {
    json::JsonValue body = json::JsonValue{};
    body["provider_name"] = provider_name;
    body["base_url"] = base_url;
    body["api_key"] = api_key;
    body["adapter"] = adapter.empty() ? "openai_compatible" : adapter;
    return _impl->PostJson("/models/api-provider", body, out_result);
}

grpc::Status AIServiceClient::DeleteApiProvider(const std::string& provider_id,
                                                json::JsonValue* out_result) {
    json::JsonValue body = json::JsonValue{};
    body["provider_id"] = provider_id;
    return _impl->PostJson("/models/api-provider/delete", body, out_result);
}

grpc::Status AIServiceClient::KbList(int32_t uid, json::JsonValue* out_result) {
    return _impl->GetJson("/kb/list?uid=" + std::to_string(uid), out_result);
}

grpc::Status AIServiceClient::KbDelete(int32_t uid,
                                        const std::string& kb_id,
                                        json::JsonValue* out_result) {
    return _impl->DeleteJson("/kb/" + AIServiceClient::Impl::UrlEncode(kb_id) + "?uid=" + std::to_string(uid), out_result);
}

grpc::Status AIServiceClient::MemoryList(int32_t uid, json::JsonValue* out_result) {
    return _impl->GetJson("/agent/memory?uid=" + std::to_string(uid), out_result);
}

grpc::Status AIServiceClient::MemoryCreate(int32_t uid,
                                           const std::string& content,
                                           json::JsonValue* out_result) {
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["content"] = content;
    return _impl->PostJson("/agent/memory", body, out_result);
}

grpc::Status AIServiceClient::MemoryDelete(int32_t uid,
                                           const std::string& memory_id,
                                           json::JsonValue* out_result) {
    return _impl->DeleteJson(
        "/agent/memory/" + AIServiceClient::Impl::UrlEncode(memory_id) + "?uid=" + std::to_string(uid),
        out_result);
}

grpc::Status AIServiceClient::AgentTaskCreate(int32_t uid,
                                              const std::string& title,
                                              const std::string& content,
                                              const std::string& session_id,
                                              const std::string& model_type,
                                              const std::string& model_name,
                                              const std::string& skill_name,
                                              const std::string& metadata_json,
                                              json::JsonValue* out_result) {
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["title"] = title;
    body["content"] = content;
    body["session_id"] = session_id;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["skill_name"] = skill_name;
    AIServiceClient::Impl::AttachMetadataJson(body, metadata_json);
    return _impl->PostJson("/agent/tasks", body, out_result);
}

grpc::Status AIServiceClient::AgentTaskList(int32_t uid,
                                            int limit,
                                            json::JsonValue* out_result) {
    return _impl->GetJson(
        "/agent/tasks?uid=" + std::to_string(uid) + "&limit=" + std::to_string(limit > 0 ? limit : 50),
        out_result);
}

grpc::Status AIServiceClient::AgentTaskGet(const std::string& task_id,
                                           json::JsonValue* out_result) {
    return _impl->GetJson("/agent/tasks/" + AIServiceClient::Impl::UrlEncode(task_id), out_result);
}

grpc::Status AIServiceClient::AgentTaskCancel(const std::string& task_id,
                                              json::JsonValue* out_result) {
    return _impl->PostJson(
        "/agent/tasks/" + AIServiceClient::Impl::UrlEncode(task_id) + "/cancel",
        json::JsonValue{},
        out_result);
}

grpc::Status AIServiceClient::AgentTaskResume(const std::string& task_id,
                                              json::JsonValue* out_result) {
    return _impl->PostJson(
        "/agent/tasks/" + AIServiceClient::Impl::UrlEncode(task_id) + "/resume",
        json::JsonValue{},
        out_result);
}

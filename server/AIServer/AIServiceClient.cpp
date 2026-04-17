#include "AIServiceClient.h"
#include "ConfigMgr.h"
#include "logging/Logger.h"
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <json/json.h>
#include <sstream>
#include <thread>
#include <future>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class AIServiceClient::Impl {
public:
    Impl() {
        auto& cfg = ConfigMgr::Inst();
        _host = cfg["AIOrchestrator"]["Host"];
        _port = cfg["AIOrchestrator"]["Port"];
        _timeout_sec = std::stoi(cfg["AIOrchestrator"]["TimeoutSec"].empty() ? "120" : cfg["AIOrchestrator"]["TimeoutSec"]);
    }

    grpc::Status PostJson(const std::string& path,
                          const Json::Value& body,
                          Json::Value* out_result) {
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);

            auto const results = resolver.resolve(_host, _port);
            stream.connect(results);

            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            std::string body_str = Json::writeString(builder, body);

            http::request<http::string_body> req{http::verb::post, path, 11};
            req.set(http::field::host, _host);
            req.set(http::field::content_type, "application/json");
            req.body() = body_str;
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);

            if (res.result() != http::status::ok) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                   "AIOrchestrator returned " + std::to_string(res.result_int()));
            }

            std::istringstream iss(res.body());
            Json::CharReaderBuilder reader;
            std::string err;
            if (!Json::parseFromStream(reader, iss, out_result, &err)) {
                return grpc::Status(grpc::StatusCode::INTERNAL, "JSON parse error: " + err);
            }

            stream.socket().close();
            return grpc::Status::OK;
        } catch (const std::exception& e) {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, e.what());
        }
    }

    grpc::Status PostJsonSSE(const std::string& path,
                              const Json::Value& body,
                              ChunkCallback on_chunk,
                              Json::Value* out_result) {
        try {
            net::io_context ioc;
            tcp::resolver resolver(ioc);
            beast::tcp_stream stream(ioc);

            auto const results = resolver.resolve(_host, _port);
            stream.connect(results);

            Json::StreamWriterBuilder builder;
            builder["indentation"] = "";
            std::string body_str = Json::writeString(builder, body);

            http::request<http::string_body> req{http::verb::post, path, 11};
            req.set(http::field::host, _host);
            req.set(http::field::content_type, "application/json");
            req.body() = body_str;
            req.prepare_payload();

            http::write(stream, req);

            beast::flat_buffer buffer;
            http::response<http::string_body> res;
            http::read(stream, buffer, res);

            std::string accumulated;
            std::string current_msg_id;
            int64_t total_tokens = 0;

            auto parse_and_emit = [&](const std::string& line) -> bool {
                if (line.size() < 6 || line.substr(0, 6) != "data: ") return false;
                std::string json_str = line.substr(6);
                if (json_str == "[DONE]") return true;

                std::istringstream iss(json_str);
                Json::CharReaderBuilder reader;
                Json::Value chunk_obj;
                std::string err;
                if (!Json::parseFromStream(reader, iss, &chunk_obj, &err)) return false;

                std::string chunk_text = chunk_obj.get("chunk", "").asString();
                bool final_flag = chunk_obj.get("is_final", false).asBool();
                if (!chunk_obj.get("msg_id", "").asString().empty()) {
                    current_msg_id = chunk_obj.get("msg_id", "").asString();
                }
                int64_t tokens = chunk_obj.get("total_tokens", 0).asInt64();
                if (tokens > 0) total_tokens = tokens;

                accumulated += chunk_text;
                on_chunk(chunk_text, final_flag, current_msg_id, total_tokens);

                if (final_flag && out_result) {
                    (*out_result)["code"] = 0;
                    (*out_result)["content"] = accumulated;
                    (*out_result)["tokens"] = static_cast<Json::Int64>(total_tokens);
                }
                return final_flag;
            };

            const std::string& body_str_ref = res.body();
            size_t pos = 0;
            while (pos < body_str_ref.size()) {
                size_t eol = body_str_ref.find('\n', pos);
                if (eol == std::string::npos) break;
                std::string line = body_str_ref.substr(pos, eol - pos);
                line.erase(line.find_last_not_of(" \t\r") + 1);
                if (!line.empty()) {
                    if (parse_and_emit(line)) break;
                }
                pos = eol + 1;
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
                                    Json::Value* out_result) {
    Json::Value body;
    body["uid"] = uid;
    body["session_id"] = session_id;
    body["content"] = content;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["stream"] = false;

    return _impl->PostJson("/chat", body, out_result);
}

grpc::Status AIServiceClient::ChatStream(int32_t uid,
                                          const std::string& session_id,
                                          const std::string& content,
                                          const std::string& model_type,
                                          const std::string& model_name,
                                          ChunkCallback on_chunk,
                                          Json::Value* out_result) {
    Json::Value body;
    body["uid"] = uid;
    body["session_id"] = session_id;
    body["content"] = content;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["stream"] = true;

    return _impl->PostJsonSSE("/chat/stream", body, on_chunk, out_result);
}

grpc::Status AIServiceClient::Smart(const std::string& feature_type,
                                     const std::string& content,
                                     const std::string& target_lang,
                                     const std::string& context_json,
                                     Json::Value* out_result) {
    Json::Value body;
    body["feature_type"] = feature_type;
    body["content"] = content;
    body["target_lang"] = target_lang;
    body["context_json"] = context_json;

    return _impl->PostJson("/smart", body, out_result);
}

grpc::Status AIServiceClient::KbUpload(int32_t uid,
                                        const std::string& file_name,
                                        const std::string& file_type,
                                        const std::string& base64_content,
                                        Json::Value* out_result) {
    Json::Value body;
    body["uid"] = uid;
    body["file_name"] = file_name;
    body["file_type"] = file_type;
    body["content"] = base64_content;

    return _impl->PostJson("/kb/upload", body, out_result);
}

grpc::Status AIServiceClient::KbSearch(int32_t uid,
                                        const std::string& query,
                                        int top_k,
                                        Json::Value* out_result) {
    Json::Value body;
    body["uid"] = uid;
    body["query"] = query;
    body["top_k"] = top_k;

    return _impl->PostJson("/kb/search", body, out_result);
}

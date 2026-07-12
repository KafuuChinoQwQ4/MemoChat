#include "AIServiceClient.hpp"
#include "AIServiceAlgorithms.hpp"
#include "ConfigMgr.hpp"
#include "logging/Logger.hpp"
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <sstream>
#include <future>
#include <chrono>
#include <limits>
#include <cctype>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <string_view>

import memochat.ai.client_algorithms;

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace json = memochat::json;
namespace client_modules = memochat::ai::client::modules;

namespace
{
std::string TrimAscii(std::string value)
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

std::string ConfigValue(std::string_view section, std::string_view key)
{
    return TrimAscii(ConfigMgr::Inst().GetValue(std::string(section), std::string(key)));
}

std::string EnvValue(const std::string& name)
{
    if (name.empty())
    {
        return "";
    }
    const char* value = std::getenv(name.c_str());
    return value == nullptr ? "" : TrimAscii(value);
}

std::string ResolveConfiguredSecret(std::string_view section,
                                    std::string_view env_key,
                                    std::string_view value_key,
                                    std::string_view default_env)
{
    std::string env_name = ConfigValue(section, env_key);
    if (env_name.empty())
    {
        env_name = std::string(default_env);
    }
    if (const std::string env_value = EnvValue(env_name); !env_value.empty())
    {
        return env_value;
    }
    return ConfigValue(section, value_key);
}

std::string AiOrchestratorInternalHeader()
{
    std::string header = ConfigValue("AIOrchestrator", "InternalAuthHeader");
    return header.empty() ? "X-MemoChat-AI-Internal-Key" : header;
}

std::string ResolveAiOrchestratorInternalKey()
{
    return ResolveConfiguredSecret("AIOrchestrator",
                                   "InternalApiKeyEnv",
                                   "InternalApiKey",
                                   "MEMOCHAT_AI_INTERNAL_API_KEY");
}

std::string ProviderAdminAuthHeader()
{
    std::string header = ConfigValue("AIProviderAdmin", "AuthHeader");
    return header.empty() ? "X-MemoChat-AI-Provider-Admin-Key" : header;
}

std::string ResolveProviderAdminKey()
{
    return ResolveConfiguredSecret("AIProviderAdmin", "AdminKeyEnv", "AdminKey", "MEMOCHAT_AI_PROVIDER_ADMIN_KEY");
}

bool IsProviderAdminPath(const std::string& path)
{
    return path == client_modules::RegisterApiProviderPath() || path == client_modules::DeleteApiProviderPath();
}

} // namespace

class AIServiceClient::Impl
{
public:
    Impl()
    {
        auto& cfg = ConfigMgr::Inst();
        _host = cfg["AIOrchestrator"]["Host"];
        _port = cfg["AIOrchestrator"]["Port"];
        const std::string raw_timeout = cfg["AIOrchestrator"]["TimeoutSec"];
        const auto parsed_timeout = client_modules::ParsePositiveIntOr(raw_timeout.data(),
                                                                       static_cast<unsigned long>(raw_timeout.size()),
                                                                       client_modules::DefaultTimeoutSec());
        _timeout_sec = parsed_timeout.value;
        if (parsed_timeout.status == client_modules::PositiveIntParseStatus::Invalid)
        {
            memolog::LogWarn("ai.client.timeout_invalid",
                             "invalid AIOrchestrator TimeoutSec; using fallback",
                             {{"configured_value", raw_timeout},
                              {"fallback_sec", std::to_string(client_modules::DefaultTimeoutSec())}});
        }
        if (ResolveAiOrchestratorInternalKey().empty())
        {
            memolog::LogWarn(
                "ai.client.internal_key_missing",
                "MEMOCHAT_AI_INTERNAL_API_KEY is not set; requests to AIOrchestrator will be rejected with 401/503",
                {});
        }
        // TODO(security): current AIOrchestrator trust model is a single shared
        // admin-level internal key (MEMOCHAT_AI_INTERNAL_API_KEY).  Future work:
        //   1. Migrate to per-account role RBAC for provider management (create/
        //      delete provider, model admin).  The admin secret gates provider
        //      mutation today but cannot enforce per-user resource isolation.
        //   2. Add per-tool allowlist for MCP/tool bridge: each caller identity
        //      should declare which tool namespaces it is permitted to invoke.
        //      Track progress in: .ai/security-audit/a/plan.md §Residual Risks.
    }

    static std::string UrlEncode(const std::string& input)
    {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex << std::uppercase;
        for (unsigned char ch : input)
        {
            if (client_modules::ShouldKeepUrlByte(std::isalnum(ch) != 0, ch))
            {
                escaped << static_cast<char>(ch);
            }
            else
            {
                escaped << '%' << std::setw(2) << static_cast<int>(ch);
            }
        }
        return escaped.str();
    }

    grpc::Status RequestJson(http::verb verb,
                             const std::string& path,
                             const std::string& body_str,
                             bool has_body,
                             json::JsonValue* out_result)
    {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        stream.expires_after(std::chrono::seconds(_timeout_sec));
        beast::error_code ec;

        const auto results = resolver.resolve(_host, _port, ec);
        if (ec)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, ec.message());
        }
        stream.connect(results, ec);
        if (ec)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, ec.message());
        }

        http::request<http::string_body> req{verb, path, client_modules::HttpVersion()};
        req.set(http::field::host, _host);
        if (const std::string internal_key = ResolveAiOrchestratorInternalKey(); !internal_key.empty())
        {
            req.set(AiOrchestratorInternalHeader(), internal_key);
        }
        if (IsProviderAdminPath(path))
        {
            if (const std::string provider_admin_key = ResolveProviderAdminKey(); !provider_admin_key.empty())
            {
                req.set(ProviderAdminAuthHeader(), provider_admin_key);
            }
        }
        if (client_modules::ShouldSetJsonBody(has_body))
        {
            req.set(http::field::content_type, client_modules::JsonContentType());
            req.body() = body_str;
        }
        req.prepare_payload();

        http::write(stream, req, ec);
        if (ec)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, ec.message());
        }

        beast::flat_buffer buffer;
        http::response<http::string_body> res;
        stream.expires_after(std::chrono::seconds(_timeout_sec));
        http::read(stream, buffer, res, ec);
        if (ec)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, ec.message());
        }

        if (res.result() != http::status::ok)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                "AIOrchestrator returned " + std::to_string(res.result_int()));
        }

        if (!memochat::json::reader_parse(res.body(), *out_result))
        {
            return grpc::Status(grpc::StatusCode::INTERNAL, "JSON parse error");
        }

        stream.socket().close(ec);
        return grpc::Status::OK;
    }

    grpc::Status PostJson(const std::string& path, const json::JsonValue& body, json::JsonValue* out_result)
    {
        std::string body_str = memochat::json::glaze_stringify(body);
        return RequestJson(http::verb::post, path, body_str, true, out_result);
    }

    grpc::Status GetJson(const std::string& path, json::JsonValue* out_result)
    {
        return RequestJson(http::verb::get, path, client_modules::EmptyBody(), false, out_result);
    }

    grpc::Status DeleteJson(const std::string& path, json::JsonValue* out_result)
    {
        return RequestJson(http::verb::delete_, path, client_modules::EmptyBody(), false, out_result);
    }

    static void AttachMetadataJson(json::JsonValue& body, const std::string& metadata_json)
    {
        if (!client_modules::ShouldAttachMetadata(metadata_json.empty()))
        {
            return;
        }
        json::JsonValue metadata = json::JsonValue{};
        const bool parse_ok = memochat::json::reader_parse(metadata_json, metadata);
        if (client_modules::ShouldUseParsedMetadata(parse_ok, metadata.isObject()))
        {
            body["metadata"] = std::move(metadata);
        }
    }

    grpc::Status PostJsonSSE(const std::string& path,
                             const json::JsonValue& body,
                             ChunkCallback on_chunk,
                             json::JsonValue* out_result)
    {
        net::io_context ioc;
        tcp::resolver resolver(ioc);
        beast::tcp_stream stream(ioc);
        stream.expires_after(std::chrono::seconds(_timeout_sec));
        beast::error_code ec;

        const auto results = resolver.resolve(_host, _port, ec);
        if (ec)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, ec.message());
        }
        stream.connect(results, ec);
        if (ec)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, ec.message());
        }

        std::string body_str = memochat::json::glaze_stringify(body);

        http::request<http::string_body> req{http::verb::post, path, client_modules::HttpVersion()};
        req.set(http::field::host, _host);
        if (const std::string internal_key = ResolveAiOrchestratorInternalKey(); !internal_key.empty())
        {
            req.set(AiOrchestratorInternalHeader(), internal_key);
        }
        req.set(http::field::content_type, client_modules::JsonContentType());
        req.body() = body_str;
        req.prepare_payload();

        http::write(stream, req, ec);
        if (ec)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, ec.message());
        }

        beast::flat_buffer buffer;
        http::response_parser<http::dynamic_body> parser;
        parser.body_limit((std::numeric_limits<std::uint64_t>::max)());
        stream.expires_after(std::chrono::seconds(_timeout_sec));

        std::string accumulated;
        std::string current_msg_id;
        int64_t total_tokens = 0;
        bool saw_final = false;

        auto parse_and_emit = [&](const std::string& line) -> bool
        {
            if (!client_modules::IsSseDataLine(line.data(), static_cast<unsigned long>(line.size())))
                return false;
            std::string json_str = line.substr(client_modules::SseDataPrefixSize());
            if (client_modules::IsSseDoneSentinel(json_str.data(), static_cast<unsigned long>(json_str.size())))
            {
                saw_final = true;
                return true;
            }

            json::JsonValue chunk_obj = json::JsonValue{};
            bool ok = memochat::json::reader_parse(json_str, chunk_obj);
            if (!ok)
                return false;

            std::string chunk_text = json::glaze_safe_get<std::string>(chunk_obj["chunk"], "");
            bool final_flag = json::glaze_safe_get<bool>(chunk_obj["is_final"], false);
            std::string msg_id = json::glaze_safe_get<std::string>(chunk_obj["msg_id"], "");
            std::string trace_id = json::glaze_safe_get<std::string>(chunk_obj["trace_id"], "");
            std::string skill = json::glaze_safe_get<std::string>(chunk_obj["skill"], "");
            std::string feedback_summary = json::glaze_safe_get<std::string>(chunk_obj["feedback_summary"], "");
            std::string observations_json = client_modules::EmptyJsonArray();
            std::string events_json = client_modules::EmptyJsonArray();
            if (memochat::json::glaze_has_key(chunk_obj, "observations"))
            {
                observations_json = memochat::json::glaze_stringify(chunk_obj["observations"].get<json::JsonValue>());
            }
            if (memochat::json::glaze_has_key(chunk_obj, "events"))
            {
                events_json = memochat::json::glaze_stringify(chunk_obj["events"].get<json::JsonValue>());
            }
            if (client_modules::ShouldUpdateCurrentMessageId(msg_id.empty()))
            {
                current_msg_id = msg_id;
            }
            int64_t tokens = json::glaze_safe_get<int64_t>(chunk_obj, "total_tokens", 0);
            if (client_modules::ShouldUpdateTotalTokens(tokens))
                total_tokens = tokens;

            accumulated += chunk_text;
            if (client_modules::ShouldEmitChunk(static_cast<bool>(on_chunk)))
            {
                on_chunk(chunk_text,
                         final_flag,
                         current_msg_id,
                         total_tokens,
                         trace_id,
                         skill,
                         feedback_summary,
                         observations_json,
                         events_json);
            }

            if (client_modules::ShouldStoreFinalChunkResult(final_flag, out_result != nullptr))
            {
                (*out_result)["code"] = client_modules::SuccessCode();
                (*out_result)["content"] = accumulated;
                (*out_result)["tokens"] = total_tokens;
                (*out_result)["trace_id"] = trace_id;
                (*out_result)["skill"] = skill;
                (*out_result)["feedback_summary"] = feedback_summary;
            }
            if (final_flag)
            {
                saw_final = true;
            }
            return final_flag;
        };

        auto trim_line = [](std::string& line)
        {
            std::size_t end = line.size();
            while (end > 0 && client_modules::IsLineTrimChar(line[end - 1]))
            {
                --end;
            }
            if (end == 0)
            {
                line.clear();
            }
            else
            {
                line.erase(end);
            }
        };

        std::string pending;
        auto consume_body = [&]() -> bool
        {
            auto& body = parser.get().body();
            if (body.size() > 0)
            {
                pending += beast::buffers_to_string(body.data());
                body.consume(body.size());
            }

            for (;;)
            {
                const auto eol = pending.find('\n');
                if (eol == std::string::npos)
                {
                    return false;
                }
                std::string line = pending.substr(0, eol);
                pending.erase(0, eol + 1);
                trim_line(line);
                if (!line.empty() && parse_and_emit(line))
                {
                    return true;
                }
            }
        };

        http::read_header(stream, buffer, parser, ec);
        if (ec)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE, ec.message());
        }
        if (parser.get().result() != http::status::ok)
        {
            return grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                "AIOrchestrator returned " + std::to_string(parser.get().result_int()));
        }

        bool done = consume_body();
        while (!done && !parser.is_done())
        {
            stream.expires_after(std::chrono::seconds(_timeout_sec));
            http::read_some(stream, buffer, parser, ec);
            if (ec)
            {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, ec.message());
            }
            done = consume_body();
        }
        if (!done && !pending.empty())
        {
            trim_line(pending);
            if (!pending.empty())
            {
                done = parse_and_emit(pending);
            }
            pending.clear();
        }
        if (client_modules::ShouldFallbackStreamResult(saw_final, out_result != nullptr))
        {
            (*out_result)["code"] = client_modules::SuccessCode();
            (*out_result)["content"] = accumulated;
            (*out_result)["tokens"] = total_tokens;
        }

        stream.socket().close(ec);
        return grpc::Status::OK;
    }

    std::string _host;
    std::string _port;
    int _timeout_sec = 120;
};

AIServiceClient::AIServiceClient()
    : _impl(std::make_unique<Impl>())
{
}

AIServiceClient::~AIServiceClient() = default;

grpc::Status AIServiceClient::Chat(int32_t uid,
                                   const std::string& session_id,
                                   const std::string& content,
                                   const std::string& model_type,
                                   const std::string& model_name,
                                   const std::string& metadata_json,
                                   json::JsonValue* out_result)
{
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["session_id"] = session_id;
    body["content"] = content;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["stream"] = false;
    Impl::AttachMetadataJson(body, metadata_json);

    return _impl->PostJson(client_modules::ChatPath(), body, out_result);
}

grpc::Status AIServiceClient::ChatStream(int32_t uid,
                                         const std::string& session_id,
                                         const std::string& content,
                                         const std::string& model_type,
                                         const std::string& model_name,
                                         const std::string& metadata_json,
                                         ChunkCallback on_chunk,
                                         json::JsonValue* out_result)
{
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["session_id"] = session_id;
    body["content"] = content;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["stream"] = true;
    Impl::AttachMetadataJson(body, metadata_json);

    return _impl->PostJsonSSE(client_modules::ChatStreamPath(), body, on_chunk, out_result);
}

grpc::Status AIServiceClient::Smart(int32_t uid,
                                    const std::string& feature_type,
                                    const std::string& content,
                                    const std::string& target_lang,
                                    const std::string& context_json,
                                    const std::string& model_type,
                                    const std::string& model_name,
                                    const std::string& deployment_preference,
                                    json::JsonValue* out_result)
{
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["feature_type"] = feature_type;
    body["content"] = content;
    body["target_lang"] = target_lang;
    body["context_json"] = context_json;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["deployment_preference"] = client_modules::ShouldUseDefaultDeploymentPreference(deployment_preference.empty())
                                        ? client_modules::DefaultDeploymentPreference()
                                        : deployment_preference;

    return _impl->PostJson(client_modules::SmartPath(), body, out_result);
}

grpc::Status AIServiceClient::KbUpload(int32_t uid,
                                       const std::string& file_name,
                                       const std::string& file_type,
                                       const std::string& base64_content,
                                       json::JsonValue* out_result)
{
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["file_name"] = file_name;
    body["file_type"] = file_type;
    body["content"] = base64_content;

    return _impl->PostJson(client_modules::KbUploadPath(), body, out_result);
}

grpc::Status AIServiceClient::KbSearch(int32_t uid, const std::string& query, int top_k, json::JsonValue* out_result)
{
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["query"] = query;
    body["top_k"] = top_k;

    return _impl->PostJson(client_modules::KbSearchPath(), body, out_result);
}

grpc::Status AIServiceClient::ListModels(json::JsonValue* out_result)
{
    return _impl->GetJson(client_modules::ModelsPath(), out_result);
}

grpc::Status AIServiceClient::RegisterApiProvider(const std::string& provider_name,
                                                  const std::string& base_url,
                                                  const std::string& api_key,
                                                  const std::string& adapter,
                                                  json::JsonValue* out_result)
{
    json::JsonValue body = json::JsonValue{};
    body["provider_name"] = provider_name;
    body["base_url"] = base_url;
    body["api_key"] = api_key;
    body["adapter"] = client_modules::ShouldUseDefaultApiProviderAdapter(adapter.empty())
                          ? client_modules::DefaultApiProviderAdapter()
                          : adapter;
    return _impl->PostJson(client_modules::RegisterApiProviderPath(), body, out_result);
}

grpc::Status AIServiceClient::DeleteApiProvider(const std::string& provider_id, json::JsonValue* out_result)
{
    json::JsonValue body = json::JsonValue{};
    body["provider_id"] = provider_id;
    return _impl->PostJson(client_modules::DeleteApiProviderPath(), body, out_result);
}

grpc::Status AIServiceClient::KbList(int32_t uid, json::JsonValue* out_result)
{
    return _impl->GetJson(std::string(client_modules::KbListPathPrefix()) + std::to_string(uid), out_result);
}

grpc::Status AIServiceClient::KbDelete(int32_t uid, const std::string& kb_id, json::JsonValue* out_result)
{
    return _impl->DeleteJson(std::string(client_modules::KbPathPrefix()) + AIServiceClient::Impl::UrlEncode(kb_id) +
                                 client_modules::UidQueryPrefix() + std::to_string(uid),
                             out_result);
}

grpc::Status AIServiceClient::MemoryList(int32_t uid, json::JsonValue* out_result)
{
    return _impl->GetJson(std::string(client_modules::AgentMemoryListPathPrefix()) + std::to_string(uid), out_result);
}

grpc::Status AIServiceClient::MemoryCreate(int32_t uid, const std::string& content, json::JsonValue* out_result)
{
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["content"] = content;
    return _impl->PostJson(client_modules::AgentMemoryPath(), body, out_result);
}

grpc::Status AIServiceClient::MemoryDelete(int32_t uid, const std::string& memory_id, json::JsonValue* out_result)
{
    return _impl->DeleteJson(std::string(client_modules::AgentMemoryItemPathPrefix()) +
                                 AIServiceClient::Impl::UrlEncode(memory_id) + client_modules::UidQueryPrefix() +
                                 std::to_string(uid),
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
                                              json::JsonValue* out_result)
{
    json::JsonValue body = json::JsonValue{};
    body["uid"] = uid;
    body["title"] = title;
    body["content"] = content;
    body["session_id"] = session_id;
    body["model_type"] = model_type;
    body["model_name"] = model_name;
    body["skill_name"] = skill_name;
    AIServiceClient::Impl::AttachMetadataJson(body, metadata_json);
    return _impl->PostJson(client_modules::AgentTasksPath(), body, out_result);
}

grpc::Status AIServiceClient::AgentTaskList(int32_t uid, int limit, json::JsonValue* out_result)
{
    return _impl->GetJson(
        std::string(client_modules::AgentTasksListPathPrefix()) + std::to_string(uid) +
            client_modules::LimitQueryPrefix() +
            std::to_string(
                ai_service_algorithms::NormalizeAgentTaskListLimit(limit, client_modules::DefaultAgentTaskLimit())),
        out_result);
}

grpc::Status AIServiceClient::AgentTaskGet(int32_t uid, const std::string& task_id, json::JsonValue* out_result)
{
    return _impl->GetJson(std::string(client_modules::AgentTasksPath()) + "/" +
                              AIServiceClient::Impl::UrlEncode(task_id) + client_modules::UidQueryPrefix() +
                              std::to_string(uid),
                          out_result);
}

grpc::Status AIServiceClient::AgentTaskCancel(int32_t uid, const std::string& task_id, json::JsonValue* out_result)
{
    return _impl->PostJson(std::string(client_modules::AgentTasksPath()) + "/" +
                               AIServiceClient::Impl::UrlEncode(task_id) + client_modules::CancelSuffix() +
                               client_modules::UidQueryPrefix() + std::to_string(uid),
                           json::JsonValue{},
                           out_result);
}

grpc::Status AIServiceClient::AgentTaskResume(int32_t uid, const std::string& task_id, json::JsonValue* out_result)
{
    return _impl->PostJson(std::string(client_modules::AgentTasksPath()) + "/" +
                               AIServiceClient::Impl::UrlEncode(task_id) + client_modules::ResumeSuffix() +
                               client_modules::UidQueryPrefix() + std::to_string(uid),
                           json::JsonValue{},
                           out_result);
}

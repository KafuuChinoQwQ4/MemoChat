#include "AIServiceCore.h"
#include "AIServiceClient.h"
#include "AIServiceJsonMapper.h"
#include "ConversationContext.h"
#include "db/AISessionRepo.h"
#include "ConfigMgr.h"
#include "logging/Logger.h"
#include <chrono>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/random_generator.hpp>


using namespace std::chrono;

AIServiceCore::AIServiceCore()
    : _ai_client(std::make_unique<AIServiceClient>()),
      _session_repo(std::make_unique<AISessionRepo>()) {}

AIServiceCore::~AIServiceCore() = default;

std::string AIServiceCore::GetOrCreateSessionId(int32_t uid, const std::string& model_type, const std::string& model_name) {
    if (!model_type.empty() && !model_name.empty()) {
        return _session_repo->Create(uid, model_type, model_name);
    }
    auto& cfg = ConfigMgr::Inst();
    std::string default_type = cfg["AI"]["DefaultBackend"];
    std::string default_model = cfg["AI"]["DefaultModel"];
    return _session_repo->Create(uid, default_type, default_model);
}

void AIServiceCore::SaveUserMessage(const std::string& session_id, int32_t uid, const std::string& content, const std::string& model_name) {
    _session_repo->SaveMessage(session_id, uid, "user", content, model_name, 0);
}

void AIServiceCore::SaveAIMessage(const std::string& session_id, int32_t uid, const std::string& content, const std::string& model_name, int64_t tokens_used) {
    _session_repo->SaveMessage(session_id, uid, "assistant", content, model_name, tokens_used);
}

grpc::Status AIServiceCore::HandleChat(const ai::AIChatReq& req, ai::AIChatRsp* reply) {
    reply->set_code(0);
    reply->set_message("ok");
    std::string session_id = req.session_id();
    if (session_id.empty()) {
        session_id = GetOrCreateSessionId(req.from_uid(), req.model_type(), req.model_name());
    }
    reply->set_session_id(session_id);
    SaveUserMessage(session_id, req.from_uid(), req.content(), req.model_name());

    memochat::json::JsonValue result;
    auto status = _ai_client->Chat(
        req.from_uid(), session_id, req.content(), req.model_type(), req.model_name(), req.metadata_json(), &result);
    if (!status.ok()) {
        reply->set_code(500);
        reply->set_message("AI orchestrator unavailable: " + status.error_message());
        return grpc::Status::OK;
    }
    if (memochat::json::glaze_has_key(result, "error") && !memochat::json::glaze_safe_get<std::string>(result["error"], "").empty()) {
        reply->set_code(400);
        reply->set_message(memochat::json::glaze_safe_get<std::string>(result["error"], ""));
        return grpc::Status::OK;
    }
    reply->set_ai_content(memochat::json::glaze_safe_get<std::string>(result["content"], ""));
    reply->set_tokens_used(memochat::json::glaze_safe_get<int>(result["tokens"], 0));
    reply->set_model_name(memochat::json::glaze_safe_get<std::string>(result["model"], req.model_name()));
    reply->set_trace_id(memochat::json::glaze_safe_get<std::string>(result["trace_id"], ""));
    reply->set_skill(memochat::json::glaze_safe_get<std::string>(result["skill"], ""));
    reply->set_feedback_summary(memochat::json::glaze_safe_get<std::string>(result["feedback_summary"], ""));
    if (auto observations = memochat::json::glaze_get_array(result["observations"])) {
        for (const auto& observation : *observations) {
            reply->add_observations(memochat::json::glaze_safe_get<std::string>(observation, ""));
        }
    }
    if (memochat::json::glaze_has_key(result, "events")) {
        reply->set_events_json(memochat::json::glaze_stringify(result["events"].get<memochat::json::JsonValue>()));
    } else {
        reply->set_events_json("[]");
    }
    SaveAIMessage(session_id, req.from_uid(), reply->ai_content(), reply->model_name(), reply->tokens_used());
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::HandleChatStream(const ai::AIChatReq& req, grpc::ServerWriter<ai::AIChatStreamChunk>* writer) {
    std::string session_id = req.session_id();
    if (session_id.empty()) {
        session_id = GetOrCreateSessionId(req.from_uid(), req.model_type(), req.model_name());
    }
    SaveUserMessage(session_id, req.from_uid(), req.content(), req.model_name());

    memochat::json::JsonValue result;
    auto status = _ai_client->ChatStream(
        req.from_uid(), session_id, req.content(), req.model_type(), req.model_name(), req.metadata_json(),
        [writer](
            const std::string& chunk,
            bool is_final,
            const std::string& msg_id,
            int64_t total_tokens,
            const std::string& trace_id,
            const std::string& skill,
            const std::string& feedback_summary,
            const std::string& observations_json,
            const std::string& events_json) {
            ai::AIChatStreamChunk out;
            out.set_chunk(chunk);
            out.set_is_final(is_final);
            out.set_msg_id(msg_id);
            out.set_total_tokens(total_tokens);
            out.set_trace_id(trace_id);
            out.set_skill(skill);
            out.set_feedback_summary(feedback_summary);
            out.set_observations_json(observations_json);
            out.set_events_json(events_json);
            writer->Write(out);
        },
        &result);
    if (!status.ok()) {
        return grpc::Status(status.error_code(), status.error_message());
    }
    if (memochat::json::glaze_has_key(result, "error") && !memochat::json::glaze_safe_get<std::string>(result["error"], "").empty()) {
        ai::AIChatStreamChunk err;
        err.set_chunk(memochat::json::glaze_safe_get<std::string>(result["error"], ""));
        err.set_is_final(true);
        writer->Write(err);
    } else {
        std::string ai_content = memochat::json::glaze_safe_get<std::string>(result["content"], "");
        std::string model_name_used = memochat::json::glaze_safe_get<std::string>(result["model"], req.model_name());
        SaveAIMessage(session_id, req.from_uid(), ai_content, model_name_used, memochat::json::glaze_safe_get<int>(result["tokens"], 0));
    }
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::HandleSmart(const ai::AISmartReq& req, ai::AISmartRsp* reply) {
    reply->set_code(0);
    reply->set_message("ok");
    memochat::json::JsonValue result;
    auto status = _ai_client->Smart(req.feature_type(), req.content(), req.target_lang(), req.context_json(), &result);
    if (!status.ok()) {
        reply->set_code(500);
        reply->set_message("AI orchestrator unavailable");
        return grpc::Status::OK;
    }
    reply->set_result(memochat::json::glaze_safe_get<std::string>(result["content"], ""));
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::GetHistory(const ai::AIHistoryReq& req, ai::AIHistoryRsp* reply) {
    reply->set_code(0);
    auto messages = _session_repo->GetMessages(req.session_id(), req.limit() > 0 ? req.limit() : 20, req.offset());
    for (auto& it : messages) {
        auto* m = reply->add_messages();
        m->set_msg_id(it.msg_id());
        m->set_role(it.role());
        m->set_content(it.content());
        m->set_created_at(it.created_at());
    }
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::CreateSession(const ai::AICreateSessionReq& req, ai::AISessionRsp* reply) {
    reply->set_code(0);
    reply->set_message("ok");
    std::string session_id = GetOrCreateSessionId(req.uid(), req.model_type(), req.model_name());
    auto info = _session_repo->GetSession(session_id);
    if (info) {
        reply->set_allocated_session(info.release());
    }
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::ListSessions(const ai::AICreateSessionReq& req, ai::AISessionRsp* reply) {
    reply->set_code(0);
    reply->set_message("ok");
    auto sessions = _session_repo->ListByUid(req.uid());
    for (auto& s : sessions) {
        reply->add_sessions()->CopyFrom(s);
    }
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::DeleteSession(const ai::AIDeleteSessionReq& req, ai::AIDeleteSessionRsp* reply) {
    reply->set_code(0);
    reply->set_message("ok");
    if (!_session_repo->SoftDelete(req.session_id())) {
        reply->set_code(404);
        reply->set_message("session not found");
    }
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::ListModels(const ai::AIListModelsReq& req, ai::AIListModelsRsp* reply) {
    (void)req;
    reply->set_code(0);
    auto& cfg = ConfigMgr::Inst();
    std::string default_model = cfg["AI"]["DefaultModel"];
    auto add_model = [&](const std::string& type, const std::string& name, const std::string& display, bool enabled, int64_t ctx_window, bool supports_thinking, bool is_default) {
        auto* m = reply->add_models();
        m->set_model_type(type);
        m->set_model_name(name);
        m->set_display_name(display);
        m->set_is_enabled(enabled);
        m->set_context_window(ctx_window);
        m->set_supports_thinking(supports_thinking);
        if (is_default) {
            reply->mutable_default_model()->CopyFrom(*m);
        }
    };

    auto add_configured_models = [&]() {
        if (cfg["Ollama"]["Enabled"] == "true") {
            add_model("ollama", "qwen3:4b", "Qwen 3 4B", true, 8192, true, default_model == "qwen3:4b");
        }
        if (cfg["OpenAI"]["Enabled"] == "true") {
            add_model("openai", "gpt-4o", "GPT-4o", true, 128000, false, default_model == "gpt-4o");
            add_model("openai", "gpt-4o-mini", "GPT-4o Mini", true, 128000, false, default_model == "gpt-4o-mini");
        }
        if (cfg["Claude"]["Enabled"] == "true") {
            add_model("claude", "claude-3-5-sonnet-20241022", "Claude 3.5 Sonnet", true, 200000, false, default_model == "claude-3-5-sonnet-20241022");
        }
        if (cfg["Kimi"]["Enabled"] == "true") {
            add_model("kimi", "moonshot-v1-8k", "Kimi 8K", true, 8192, false, default_model == "moonshot-v1-8k");
        }
    };

    memochat::json::JsonValue result;
    auto status = _ai_client->ListModels(&result);
    if (status.ok() && ai_service_json_mapper::PopulateModelListFromJson(result, reply)) {
        return grpc::Status::OK;
    }

    reply->clear_models();
    reply->clear_default_model();
    add_configured_models();
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::RegisterApiProvider(const ai::AIRegisterApiProviderReq& req, ai::AIRegisterApiProviderRsp* reply) {
    reply->set_code(0);
    reply->set_message("ok");

    memochat::json::JsonValue result;
    auto status = _ai_client->RegisterApiProvider(
        req.provider_name(),
        req.base_url(),
        req.api_key(),
        req.adapter().empty() ? "openai_compatible" : req.adapter(),
        &result);
    if (!status.ok()) {
        reply->set_code(500);
        reply->set_message("AI orchestrator unavailable: " + status.error_message());
        return grpc::Status::OK;
    }

    ai_service_json_mapper::PopulateRegisterApiProviderFromJson(result, reply);
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::HandleKbUpload(const ai::AIKbUploadReq& req, ai::AIKbUploadRsp* reply) {
    reply->set_code(0);
    reply->set_message("ok");
    std::string content_b64(req.content().begin(), req.content().end());
    memochat::json::JsonValue result;
    auto status = _ai_client->KbUpload(req.uid(), req.file_name(), req.file_type(), content_b64, &result);
    if (!status.ok()) {
        reply->set_code(500);
        reply->set_message("upload failed");
        return grpc::Status::OK;
    }
    reply->set_chunks(memochat::json::glaze_safe_get<int>(result["chunks"], 0));
    reply->set_kb_id(memochat::json::glaze_safe_get<std::string>(result["kb_id"], ""));
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::HandleKbSearch(const ai::AIKbSearchReq& req, ai::AIKbSearchRsp* reply) {
    reply->set_code(0);
    memochat::json::JsonValue result;
    auto status = _ai_client->KbSearch(req.uid(), req.query(), req.top_k(), &result);
    if (!status.ok()) {
        reply->set_code(500);
        reply->set_message("search failed");
        return grpc::Status::OK;
    }
    auto chunks = memochat::json::glaze_get_array(result["chunks"]);
    if (chunks) {
        for (const auto& chunk_item : *chunks) {
            memochat::json::JsonValue chunk(chunk_item);
            auto* c = reply->add_chunks();
            c->set_content(memochat::json::glaze_safe_get<std::string>(chunk["content"], ""));
            c->set_score(static_cast<float>(memochat::json::glaze_safe_get<double>(chunk["score"], 0.0)));
            c->set_source(memochat::json::glaze_safe_get<std::string>(chunk["source"], ""));
        }
    }
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::ListKb(const ai::AIKbListReq& req, ai::AIKbListRsp* reply) {
    reply->set_code(0);
    memochat::json::JsonValue result;
    auto status = _ai_client->KbList(req.uid(), &result);
    if (!status.ok()) {
        reply->set_code(500);
        return grpc::Status::OK;
    }
    ai_service_json_mapper::PopulateKbListFromJson(result, reply);
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::DeleteKb(const ai::AIKbDeleteReq& req, ai::AIKbDeleteRsp* reply) {
    reply->set_code(0);
    reply->set_message("ok");
    memochat::json::JsonValue result;
    auto status = _ai_client->KbDelete(req.uid(), req.kb_id(), &result);
    if (!status.ok()) {
        reply->set_code(500);
        reply->set_message("delete failed");
        return grpc::Status::OK;
    }
    ai_service_json_mapper::PopulateKbDeleteFromJson(result, reply);
    return grpc::Status::OK;
}

grpc::Status AIServiceCore::HandleConfirm(const ai::AIConfirmReq& req, ai::AIConfirmRsp* reply) {
    reply->set_code(0);
    reply->set_message("ok");
    return grpc::Status::OK;
}

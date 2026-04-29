#include "AIServiceClient.h"
#include "logging/Logger.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

using namespace memochat::json;

class AIServiceClient::Impl {
public:
    Impl()
        : _channel(grpc::CreateChannel("127.0.0.1:8095", grpc::InsecureChannelCredentials()))
        , _stub(ai::AIService::NewStub(_channel)) {}

    grpc::Status makeChatCall(int32_t uid, const std::string& session_id,
                              const std::string& content,
                              const std::string& model_type,
                              const std::string& model_name,
                              const std::string& metadata_json,
                              ai::AIChatRsp* reply) {
        grpc::ClientContext ctx;
        ai::AIChatReq req;
        req.set_from_uid(uid);
        req.set_session_id(session_id);
        req.set_content(content);
        req.set_model_type(model_type);
        req.set_model_name(model_name);
        req.set_metadata_json(metadata_json);
        return _stub->Chat(&ctx, req, reply);
    }

    grpc::Status makeSmartCall(const std::string& feature_type,
                              const std::string& content,
                              const std::string& target_lang,
                              const std::string& context_json,
                              ai::AISmartRsp* reply) {
        grpc::ClientContext ctx;
        ai::AISmartReq req;
        req.set_feature_type(feature_type);
        req.set_content(content);
        req.set_target_lang(target_lang);
        req.set_context_json(context_json);
        return _stub->Smart(&ctx, req, reply);
    }

    grpc::Status makeGetHistoryCall(int32_t uid, const std::string& session_id,
                                  int limit, int offset,
                                  ai::AIHistoryRsp* reply) {
        grpc::ClientContext ctx;
        ai::AIHistoryReq req;
        req.set_from_uid(uid);
        req.set_session_id(session_id);
        req.set_limit(limit);
        req.set_offset(offset);
        return _stub->GetHistory(&ctx, req, reply);
    }

    grpc::Status makeCreateSessionCall(int32_t uid, const std::string& model_type,
                                     const std::string& model_name,
                                     ai::AISessionRsp* reply) {
        grpc::ClientContext ctx;
        ai::AICreateSessionReq req;
        req.set_uid(uid);
        req.set_model_type(model_type);
        req.set_model_name(model_name);
        return _stub->CreateSession(&ctx, req, reply);
    }

    grpc::Status makeListSessionsCall(int32_t uid, const std::string& model_type,
                                    const std::string& model_name,
                                    ai::AISessionRsp* reply) {
        grpc::ClientContext ctx;
        ai::AICreateSessionReq req;
        req.set_uid(uid);
        req.set_model_type(model_type);
        req.set_model_name(model_name);
        return _stub->ListSessions(&ctx, req, reply);
    }

    grpc::Status makeDeleteSessionCall(int32_t uid, const std::string& session_id,
                                     ai::AIDeleteSessionRsp* reply) {
        grpc::ClientContext ctx;
        ai::AIDeleteSessionReq req;
        req.set_uid(uid);
        req.set_session_id(session_id);
        return _stub->DeleteSession(&ctx, req, reply);
    }

    grpc::Status makeListModelsCall(ai::AIListModelsRsp* reply) {
        grpc::ClientContext ctx;
        ai::AIListModelsReq req;
        return _stub->ListModels(&ctx, req, reply);
    }

    grpc::Status makeRegisterApiProviderCall(const std::string& provider_name,
                                             const std::string& base_url,
                                             const std::string& api_key,
                                             const std::string& adapter,
                                             ai::AIRegisterApiProviderRsp* reply) {
        grpc::ClientContext ctx;
        ai::AIRegisterApiProviderReq req;
        req.set_provider_name(provider_name);
        req.set_base_url(base_url);
        req.set_api_key(api_key);
        req.set_adapter(adapter.empty() ? "openai_compatible" : adapter);
        return _stub->RegisterApiProvider(&ctx, req, reply);
    }

    grpc::Status makeKbUploadCall(int32_t uid, const std::string& file_name,
                                 const std::string& file_type,
                                 const std::string& base64_content,
                                 ai::AIKbUploadRsp* reply) {
        grpc::ClientContext ctx;
        ai::AIKbUploadReq req;
        req.set_uid(uid);
        req.set_file_name(file_name);
        req.set_file_type(file_type);
        req.set_content(base64_content);
        return _stub->KbUpload(&ctx, req, reply);
    }

    grpc::Status makeKbSearchCall(int32_t uid, const std::string& query,
                                 int top_k, ai::AIKbSearchRsp* reply) {
        grpc::ClientContext ctx;
        ai::AIKbSearchReq req;
        req.set_uid(uid);
        req.set_query(query);
        req.set_top_k(top_k);
        return _stub->KbSearch(&ctx, req, reply);
    }

    grpc::Status makeListKbCall(int32_t uid, ai::AIKbListRsp* reply) {
        grpc::ClientContext ctx;
        ai::AIKbListReq req;
        req.set_uid(uid);
        return _stub->KbList(&ctx, req, reply);
    }

    grpc::Status makeDeleteKbCall(int32_t uid, const std::string& kb_id,
                                 ai::AIKbDeleteRsp* reply) {
        grpc::ClientContext ctx;
        ai::AIKbDeleteReq req;
        req.set_uid(uid);
        req.set_kb_id(kb_id);
        return _stub->KbDelete(&ctx, req, reply);
    }

    std::shared_ptr<grpc::Channel> _channel;
    std::unique_ptr<ai::AIService::Stub> _stub;
};

AIServiceClient::AIServiceClient()
    : _impl(std::make_unique<Impl>()) {}

AIServiceClient::~AIServiceClient() = default;

memochat::json::JsonValue AIServiceClient::Chat(int32_t uid,
                                  const std::string& session_id,
                                  const std::string& content,
                                  const std::string& model_type,
                                  const std::string& model_name,
                                  const std::string& metadata_json) {
    ai::AIChatRsp reply;
    auto status = _impl->makeChatCall(uid, session_id, content, model_type, model_name, metadata_json, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "AIServer unavailable";
        std::map<std::string, std::string> fields = {
            {"uid", std::to_string(uid)},
            {"error", status.error_message()}
        };
        memolog::LogError("gate.ai.chat.grpc_failed", "AIServer gRPC call failed", fields);
        return root;
    }

    root["code"] = reply.code();
    root["message"] = reply.message();
    root["session_id"] = reply.session_id();
    root["content"] = reply.ai_content();
    root["tokens"] = static_cast<double>(reply.tokens_used());
    root["model"] = reply.model_name();
    root["trace_id"] = reply.trace_id();
    root["skill"] = reply.skill();
    root["feedback_summary"] = reply.feedback_summary();
    array_t observations;
    for (const auto& observation : reply.observations()) {
        observations.push_back(observation);
    }
    root["observations"] = std::move(observations);
    memochat::json::JsonValue events;
    if (!reply.events_json().empty() && memochat::json::reader_parse(reply.events_json(), events)) {
        root["events"] = events;
    } else {
        root["events"] = array_t{};
    }
    return root;
}

void AIServiceClient::ChatStream(int32_t uid,
                                 const std::string& session_id,
                                 const std::string& content,
                                 const std::string& model_type,
                                 const std::string& model_name,
                                 const std::string& metadata_json,
                                 ChunkCallback on_chunk,
                                 memochat::json::JsonValue* out_result) {
    grpc::ClientContext ctx;
    ai::AIChatStreamReq req;
    req.mutable_req()->set_from_uid(uid);
    req.mutable_req()->set_session_id(session_id);
    req.mutable_req()->set_content(content);
    req.mutable_req()->set_model_type(model_type);
    req.mutable_req()->set_model_name(model_name);
    req.mutable_req()->set_metadata_json(metadata_json);

    auto reader = _impl->_stub->ChatStream(&ctx, req);

    ai::AIChatStreamChunk chunk;
    while (reader->Read(&chunk)) {
        if (on_chunk) {
            on_chunk(
                chunk.chunk(),
                chunk.is_final(),
                chunk.msg_id(),
                chunk.total_tokens(),
                chunk.trace_id(),
                chunk.skill(),
                chunk.feedback_summary(),
                chunk.observations_json(),
                chunk.events_json());
        }
        if (chunk.is_final() && out_result) {
            (*out_result)["code"] = 0;
            (*out_result)["msg_id"] = chunk.msg_id();
            (*out_result)["total_tokens"] = static_cast<double>(chunk.total_tokens());
            (*out_result)["trace_id"] = chunk.trace_id();
            (*out_result)["skill"] = chunk.skill();
            (*out_result)["feedback_summary"] = chunk.feedback_summary();
        }
    }
    auto status = reader->Finish();
    if (!status.ok()) {
        memolog::LogError("gate.ai.chat_stream.grpc_failed", "AIServer stream gRPC call failed",
            {{"uid", std::to_string(uid)}, {"error", status.error_message()}});
        if (on_chunk) {
            on_chunk("AIServer unavailable: " + status.error_message(), true, "", 0, "", "", "", "[]", "[]");
        }
        if (out_result) {
            (*out_result)["code"] = 500;
            (*out_result)["message"] = "AIServer unavailable";
        }
    }
}

memochat::json::JsonValue AIServiceClient::Smart(const std::string& feature_type,
                                  const std::string& content,
                                  const std::string& target_lang,
                                  const std::string& context_json) {
    ai::AISmartRsp reply;
    auto status = _impl->makeSmartCall(feature_type, content, target_lang, context_json, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "AIServer unavailable";
        return root;
    }

    root["code"] = reply.code();
    root["message"] = reply.message();
    root["result"] = reply.result();
    return root;
}

memochat::json::JsonValue AIServiceClient::GetHistory(int32_t uid, const std::string& session_id,
                                      int limit, int offset) {
    ai::AIHistoryRsp reply;
    auto status = _impl->makeGetHistoryCall(uid, session_id, limit, offset, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "AIServer unavailable";
        return root;
    }

    root["code"] = reply.code();
    array_t messages;
    for (const auto& msg : reply.messages()) {
        memochat::json::JsonValue m;
        m["msg_id"] = msg.msg_id();
        m["role"] = msg.role();
        m["content"] = msg.content();
        m["created_at"] = static_cast<double>(msg.created_at());
        messages.push_back(m.impl());
    }
    root["messages"] = std::move(messages);
    return root;
}

memochat::json::JsonValue AIServiceClient::CreateSession(int32_t uid, const std::string& model_type,
                                          const std::string& model_name) {
    ai::AISessionRsp reply;
    auto status = _impl->makeCreateSessionCall(uid, model_type, model_name, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "AIServer unavailable";
        return root;
    }

    root["code"] = reply.code();
    root["message"] = reply.message();
    if (reply.has_session()) {
        const auto& s = reply.session();
        memochat::json::JsonValue session;
        session["session_id"] = s.session_id();
        session["title"] = s.title();
        session["model_type"] = s.model_type();
        session["model_name"] = s.model_name();
        session["created_at"] = static_cast<double>(s.created_at());
        session["updated_at"] = static_cast<double>(s.updated_at());
        root["session"] = std::move(session);
    }
    return root;
}

memochat::json::JsonValue AIServiceClient::ListSessions(int32_t uid, const std::string& model_type,
                                         const std::string& model_name) {
    ai::AISessionRsp reply;
    auto status = _impl->makeListSessionsCall(uid, model_type, model_name, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "AIServer unavailable";
        return root;
    }

    root["code"] = reply.code();
    root["message"] = reply.message();
    array_t sessions;
    for (const auto& s : reply.sessions()) {
        memochat::json::JsonValue sess;
        sess["session_id"] = s.session_id();
        sess["title"] = s.title();
        sess["model_type"] = s.model_type();
        sess["model_name"] = s.model_name();
        sess["created_at"] = static_cast<double>(s.created_at());
        sess["updated_at"] = static_cast<double>(s.updated_at());
        sessions.push_back(sess.impl());
    }
    root["sessions"] = std::move(sessions);
    return root;
}

memochat::json::JsonValue AIServiceClient::DeleteSession(int32_t uid, const std::string& session_id) {
    ai::AIDeleteSessionRsp reply;
    auto status = _impl->makeDeleteSessionCall(uid, session_id, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "AIServer unavailable";
        return root;
    }

    root["code"] = reply.code();
    root["message"] = reply.message();
    return root;
}

memochat::json::JsonValue AIServiceClient::ListModels() {
    ai::AIListModelsRsp reply;
    auto status = _impl->makeListModelsCall(&reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "AIServer unavailable";
        return root;
    }

    root["code"] = reply.code();
    array_t models;
    for (const auto& m : reply.models()) {
        memochat::json::JsonValue model;
        model["model_type"] = m.model_type();
        model["model_name"] = m.model_name();
        model["display_name"] = m.display_name();
        model["is_enabled"] = m.is_enabled();
        model["context_window"] = static_cast<double>(m.context_window());
        model["supports_thinking"] = m.supports_thinking();
        models.push_back(model.impl());
    }
    root["models"] = std::move(models);
    if (reply.has_default_model()) {
        const auto& dm = reply.default_model();
        memochat::json::JsonValue default_model;
        default_model["model_type"] = dm.model_type();
        default_model["model_name"] = dm.model_name();
        default_model["display_name"] = dm.display_name();
        default_model["is_enabled"] = dm.is_enabled();
        default_model["context_window"] = static_cast<double>(dm.context_window());
        default_model["supports_thinking"] = dm.supports_thinking();
        root["default_model"] = std::move(default_model);
    }
    return root;
}

memochat::json::JsonValue AIServiceClient::RegisterApiProvider(const std::string& provider_name,
                                                               const std::string& base_url,
                                                               const std::string& api_key,
                                                               const std::string& adapter) {
    ai::AIRegisterApiProviderRsp reply;
    auto status = _impl->makeRegisterApiProviderCall(provider_name, base_url, api_key, adapter, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "AIServer unavailable";
        return root;
    }

    root["code"] = reply.code();
    root["message"] = reply.message();
    root["provider_id"] = reply.provider_id();
    array_t models;
    for (const auto& m : reply.models()) {
        memochat::json::JsonValue model;
        model["model_type"] = m.model_type();
        model["model_name"] = m.model_name();
        model["display_name"] = m.display_name();
        model["is_enabled"] = m.is_enabled();
        model["context_window"] = static_cast<double>(m.context_window());
        model["supports_thinking"] = m.supports_thinking();
        models.push_back(model.impl());
    }
    root["models"] = std::move(models);
    return root;
}

memochat::json::JsonValue AIServiceClient::KbUpload(int32_t uid, const std::string& file_name,
                                     const std::string& file_type,
                                     const std::string& base64_content) {
    ai::AIKbUploadRsp reply;
    auto status = _impl->makeKbUploadCall(uid, file_name, file_type, base64_content, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "upload failed";
        return root;
    }

    root["code"] = reply.code();
    root["message"] = reply.message();
    root["chunks"] = reply.chunks();
    root["kb_id"] = reply.kb_id();
    return root;
}

memochat::json::JsonValue AIServiceClient::KbSearch(int32_t uid, const std::string& query, int top_k) {
    ai::AIKbSearchRsp reply;
    auto status = _impl->makeKbSearchCall(uid, query, top_k, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "search failed";
        return root;
    }

    root["code"] = reply.code();
    array_t chunks;
    for (const auto& c : reply.chunks()) {
        memochat::json::JsonValue chunk;
        chunk["content"] = c.content();
        chunk["score"] = c.score();
        chunk["source"] = c.source();
        chunks.push_back(chunk.impl());
    }
    root["chunks"] = std::move(chunks);
    return root;
}

memochat::json::JsonValue AIServiceClient::ListKb(int32_t uid) {
    ai::AIKbListRsp reply;
    auto status = _impl->makeListKbCall(uid, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "list failed";
        return root;
    }

    root["code"] = reply.code();
    array_t kbs;
    for (const auto& kb : reply.knowledge_bases()) {
        memochat::json::JsonValue k;
        k["kb_id"] = kb.kb_id();
        k["name"] = kb.name();
        k["chunk_count"] = kb.chunk_count();
        k["created_at"] = static_cast<double>(kb.created_at());
        k["status"] = kb.status();
        kbs.push_back(k.impl());
    }
    root["knowledge_bases"] = std::move(kbs);
    return root;
}

memochat::json::JsonValue AIServiceClient::DeleteKb(int32_t uid, const std::string& kb_id) {
    ai::AIKbDeleteRsp reply;
    auto status = _impl->makeDeleteKbCall(uid, kb_id, &reply);

    memochat::json::JsonValue root;
    if (!status.ok()) {
        root["code"] = 500;
        root["message"] = "delete failed";
        return root;
    }

    root["code"] = reply.code();
    root["message"] = reply.message();
    return root;
}

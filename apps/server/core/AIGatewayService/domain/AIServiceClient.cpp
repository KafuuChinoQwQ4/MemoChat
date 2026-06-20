#include "AIServiceClient.h"
#include "services/ai/AIPublicDtos.h"
#include "ConfigMgr.h"
#include "logging/Logger.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

using namespace memochat::json;

namespace
{

memochat::gate::services::ai::AISimpleResponseDto MakeSimpleResponse(int code, const std::string& message)
{
    memochat::gate::services::ai::AISimpleResponseDto response;
    response.code = code;
    response.message = message;
    return response;
}

memochat::gate::services::ai::AIModelInfoResponseDto ModelInfoToResponseDto(const ai::ModelInfo& model)
{
    memochat::gate::services::ai::AIModelInfoResponseDto response;
    response.model_type = model.model_type();
    response.model_name = model.model_name();
    response.display_name = model.display_name();
    response.is_enabled = model.is_enabled();
    response.context_window = static_cast<int64_t>(model.context_window());
    response.supports_thinking = model.supports_thinking();
    return response;
}

memochat::gate::services::ai::AIKnowledgeBaseInfoResponseDto KnowledgeBaseToResponseDto(
    const ai::AIKbInfo& kb)
{
    memochat::gate::services::ai::AIKnowledgeBaseInfoResponseDto response;
    response.kb_id = kb.kb_id();
    response.name = kb.name();
    response.chunk_count = kb.chunk_count();
    response.created_at = static_cast<int64_t>(kb.created_at());
    response.status = kb.status();
    return response;
}

memochat::gate::services::ai::AISessionInfoResponseDto SessionInfoToResponseDto(const ai::AISessionInfo& s)
{
    memochat::gate::services::ai::AISessionInfoResponseDto response;
    response.session_id = s.session_id();
    response.title = s.title();
    response.model_type = s.model_type();
    response.model_name = s.model_name();
    response.created_at = static_cast<double>(s.created_at());
    response.updated_at = static_cast<double>(s.updated_at());
    return response;
}

} // namespace

class AIServiceClient::Impl
{
public:
    Impl()
        : _channel(grpc::CreateChannel(AIServerAddress(), grpc::InsecureChannelCredentials()))
        , _stub(ai::AIService::NewStub(_channel))
    {
    }

    static std::string AIServerAddress()
    {
        auto& cfg = ConfigMgr::Inst();
        std::string host = cfg["AIServer"]["Host"];
        std::string port = cfg["AIServer"]["Port"];
        if (host.empty())
        {
            host = "127.0.0.1";
        }
        if (port.empty())
        {
            port = "8095";
        }
        return host + ":" + port;
    }

    grpc::Status makeChatCall(int32_t uid,
                              const std::string& session_id,
                              const std::string& content,
                              const std::string& model_type,
                              const std::string& model_name,
                              const std::string& metadata_json,
                              ai::AIChatRsp* reply)
    {
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

    grpc::Status makeSmartCall(int32_t uid,
                               const std::string& feature_type,
                               const std::string& content,
                               const std::string& target_lang,
                               const std::string& context_json,
                               const std::string& model_type,
                               const std::string& model_name,
                               const std::string& deployment_preference,
                               ai::AISmartRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AISmartReq req;
        req.set_from_uid(uid);
        req.set_feature_type(feature_type);
        req.set_content(content);
        req.set_target_lang(target_lang);
        req.set_context_json(context_json);
        req.set_model_type(model_type);
        req.set_model_name(model_name);
        req.set_deployment_preference(deployment_preference);
        return _stub->Smart(&ctx, req, reply);
    }

    grpc::Status
    makeGetHistoryCall(int32_t uid, const std::string& session_id, int limit, int offset, ai::AIHistoryRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIHistoryReq req;
        req.set_from_uid(uid);
        req.set_session_id(session_id);
        req.set_limit(limit);
        req.set_offset(offset);
        return _stub->GetHistory(&ctx, req, reply);
    }

    grpc::Status makeCreateSessionCall(int32_t uid,
                                       const std::string& model_type,
                                       const std::string& model_name,
                                       ai::AISessionRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AICreateSessionReq req;
        req.set_uid(uid);
        req.set_model_type(model_type);
        req.set_model_name(model_name);
        return _stub->CreateSession(&ctx, req, reply);
    }

    grpc::Status makeListSessionsCall(int32_t uid,
                                      const std::string& model_type,
                                      const std::string& model_name,
                                      ai::AISessionRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AICreateSessionReq req;
        req.set_uid(uid);
        req.set_model_type(model_type);
        req.set_model_name(model_name);
        return _stub->ListSessions(&ctx, req, reply);
    }

    grpc::Status makeDeleteSessionCall(int32_t uid, const std::string& session_id, ai::AIDeleteSessionRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIDeleteSessionReq req;
        req.set_uid(uid);
        req.set_session_id(session_id);
        return _stub->DeleteSession(&ctx, req, reply);
    }

    grpc::Status
    makeUpdateSessionCall(int32_t uid, const std::string& session_id, const std::string& title, ai::AISessionRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIUpdateSessionReq req;
        req.set_uid(uid);
        req.set_session_id(session_id);
        req.set_title(title);
        return _stub->UpdateSession(&ctx, req, reply);
    }

    grpc::Status makeListModelsCall(ai::AIListModelsRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIListModelsReq req;
        return _stub->ListModels(&ctx, req, reply);
    }

    grpc::Status makeRegisterApiProviderCall(const std::string& provider_name,
                                             const std::string& base_url,
                                             const std::string& api_key,
                                             const std::string& adapter,
                                             ai::AIRegisterApiProviderRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIRegisterApiProviderReq req;
        req.set_provider_name(provider_name);
        req.set_base_url(base_url);
        req.set_api_key(api_key);
        req.set_adapter(adapter.empty() ? "openai_compatible" : adapter);
        return _stub->RegisterApiProvider(&ctx, req, reply);
    }

    grpc::Status makeDeleteApiProviderCall(const std::string& provider_id, ai::AIDeleteApiProviderRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIDeleteApiProviderReq req;
        req.set_provider_id(provider_id);
        return _stub->DeleteApiProvider(&ctx, req, reply);
    }

    grpc::Status makeKbUploadCall(int32_t uid,
                                  const std::string& file_name,
                                  const std::string& file_type,
                                  const std::string& base64_content,
                                  ai::AIKbUploadRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIKbUploadReq req;
        req.set_uid(uid);
        req.set_file_name(file_name);
        req.set_file_type(file_type);
        req.set_content(base64_content);
        return _stub->KbUpload(&ctx, req, reply);
    }

    grpc::Status makeKbSearchCall(int32_t uid, const std::string& query, int top_k, ai::AIKbSearchRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIKbSearchReq req;
        req.set_uid(uid);
        req.set_query(query);
        req.set_top_k(top_k);
        return _stub->KbSearch(&ctx, req, reply);
    }

    grpc::Status makeListKbCall(int32_t uid, ai::AIKbListRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIKbListReq req;
        req.set_uid(uid);
        return _stub->KbList(&ctx, req, reply);
    }

    grpc::Status makeDeleteKbCall(int32_t uid, const std::string& kb_id, ai::AIKbDeleteRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIKbDeleteReq req;
        req.set_uid(uid);
        req.set_kb_id(kb_id);
        return _stub->KbDelete(&ctx, req, reply);
    }

    grpc::Status makeMemoryListCall(int32_t uid, ai::AIMemoryListRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIMemoryReq req;
        req.set_uid(uid);
        return _stub->MemoryList(&ctx, req, reply);
    }

    grpc::Status makeMemoryCreateCall(int32_t uid, const std::string& content, ai::AIMemoryRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIMemoryReq req;
        req.set_uid(uid);
        req.set_content(content);
        return _stub->MemoryCreate(&ctx, req, reply);
    }

    grpc::Status makeMemoryDeleteCall(int32_t uid, const std::string& memory_id, ai::AIMemoryRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIMemoryReq req;
        req.set_uid(uid);
        req.set_memory_id(memory_id);
        return _stub->MemoryDelete(&ctx, req, reply);
    }

    grpc::Status makeAgentTaskCreateCall(int32_t uid,
                                         const std::string& title,
                                         const std::string& content,
                                         const std::string& session_id,
                                         const std::string& model_type,
                                         const std::string& model_name,
                                         const std::string& skill_name,
                                         const std::string& metadata_json,
                                         ai::AIAgentTaskRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIAgentTaskReq req;
        req.set_uid(uid);
        req.set_title(title);
        req.set_content(content);
        req.set_session_id(session_id);
        req.set_model_type(model_type);
        req.set_model_name(model_name);
        req.set_skill_name(skill_name);
        req.set_metadata_json(metadata_json);
        return _stub->AgentTaskCreate(&ctx, req, reply);
    }

    grpc::Status makeAgentTaskListCall(int32_t uid, int limit, ai::AIAgentTaskRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIAgentTaskReq req;
        req.set_uid(uid);
        req.set_limit(limit);
        return _stub->AgentTaskList(&ctx, req, reply);
    }

    grpc::Status makeAgentTaskGetCall(const std::string& task_id, ai::AIAgentTaskRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIAgentTaskReq req;
        req.set_task_id(task_id);
        return _stub->AgentTaskGet(&ctx, req, reply);
    }

    grpc::Status makeAgentTaskCancelCall(const std::string& task_id, ai::AIAgentTaskRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIAgentTaskReq req;
        req.set_task_id(task_id);
        return _stub->AgentTaskCancel(&ctx, req, reply);
    }

    grpc::Status makeAgentTaskResumeCall(const std::string& task_id, ai::AIAgentTaskRsp* reply)
    {
        grpc::ClientContext ctx;
        ai::AIAgentTaskReq req;
        req.set_task_id(task_id);
        return _stub->AgentTaskResume(&ctx, req, reply);
    }

    std::shared_ptr<grpc::Channel> _channel;
    std::unique_ptr<ai::AIService::Stub> _stub;
};

AIServiceClient::AIServiceClient()
    : _impl(std::make_unique<Impl>())
{
}

AIServiceClient::~AIServiceClient() = default;

memochat::json::JsonValue AIServiceClient::Chat(int32_t uid,
                                                const std::string& session_id,
                                                const std::string& content,
                                                const std::string& model_type,
                                                const std::string& model_name,
                                                const std::string& metadata_json)
{
    ai::AIChatRsp reply;
    auto status = _impl->makeChatCall(uid, session_id, content, model_type, model_name, metadata_json, &reply);

    if (!status.ok())
    {
        std::map<std::string, std::string> fields = {{"uid", std::to_string(uid)}, {"error", status.error_message()}};
        memolog::LogError("gate.ai.chat.grpc_failed", "AIServer gRPC call failed", fields);
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "AIServer unavailable"));
    }

    memochat::gate::services::ai::AIChatResponseDto response;
    response.code = reply.code();
    response.message = reply.message();
    response.session_id = reply.session_id();
    response.content = reply.ai_content();
    response.tokens = static_cast<double>(reply.tokens_used());
    response.model = reply.model_name();
    response.trace_id = reply.trace_id();
    response.skill = reply.skill();
    response.feedback_summary = reply.feedback_summary();
    for (const auto& observation : reply.observations())
    {
        response.observations.push_back(observation);
    }
    return memochat::gate::services::ai::AIChatResponseToJsonValue(response, reply.events_json());
}

void AIServiceClient::ChatStream(int32_t uid,
                                 const std::string& session_id,
                                 const std::string& content,
                                 const std::string& model_type,
                                 const std::string& model_name,
                                 const std::string& metadata_json,
                                 ChunkCallback on_chunk,
                                 memochat::json::JsonValue* out_result)
{
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
    while (reader->Read(&chunk))
    {
        if (on_chunk)
        {
            on_chunk(chunk.chunk(),
                     chunk.is_final(),
                     chunk.msg_id(),
                     chunk.total_tokens(),
                     chunk.trace_id(),
                     chunk.skill(),
                     chunk.feedback_summary(),
                     chunk.observations_json(),
                     chunk.events_json());
        }
        if (chunk.is_final() && out_result)
        {
            (*out_result)["code"] = 0;
            (*out_result)["msg_id"] = chunk.msg_id();
            (*out_result)["total_tokens"] = static_cast<double>(chunk.total_tokens());
            (*out_result)["trace_id"] = chunk.trace_id();
            (*out_result)["skill"] = chunk.skill();
            (*out_result)["feedback_summary"] = chunk.feedback_summary();
        }
    }
    auto status = reader->Finish();
    if (!status.ok())
    {
        memolog::LogError("gate.ai.chat_stream.grpc_failed",
                          "AIServer stream gRPC call failed",
                          {{"uid", std::to_string(uid)}, {"error", status.error_message()}});
        if (on_chunk)
        {
            on_chunk("AIServer unavailable: " + status.error_message(), true, "", 0, "", "", "", "[]", "[]");
        }
        if (out_result)
        {
            (*out_result)["code"] = 500;
            (*out_result)["message"] = "AIServer unavailable";
        }
    }
}

memochat::json::JsonValue AIServiceClient::Smart(int32_t uid,
                                                 const std::string& feature_type,
                                                 const std::string& content,
                                                 const std::string& target_lang,
                                                 const std::string& context_json,
                                                 const std::string& model_type,
                                                 const std::string& model_name,
                                                 const std::string& deployment_preference)
{
    ai::AISmartRsp reply;
    auto status = _impl->makeSmartCall(uid,
                                       feature_type,
                                       content,
                                       target_lang,
                                       context_json,
                                       model_type,
                                       model_name,
                                       deployment_preference,
                                       &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "AIServer unavailable"));
    }

    memochat::gate::services::ai::AISmartResponseDto response;
    response.code = reply.code();
    response.message = reply.message();
    response.result = reply.result();
    return memochat::gate::services::ai::AISmartResponseToJsonValue(response);
}

memochat::json::JsonValue AIServiceClient::GetHistory(int32_t uid, const std::string& session_id, int limit, int offset)
{
    ai::AIHistoryRsp reply;
    auto status = _impl->makeGetHistoryCall(uid, session_id, limit, offset, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "AIServer unavailable"));
    }

    memochat::gate::services::ai::AIHistoryResponseDto response;
    response.code = reply.code();
    for (const auto& msg : reply.messages())
    {
        memochat::gate::services::ai::AIHistoryMessageResponseDto item;
        item.msg_id = msg.msg_id();
        item.role = msg.role();
        item.content = msg.content();
        item.created_at = static_cast<double>(msg.created_at());
        response.messages.push_back(std::move(item));
    }
    return memochat::gate::services::ai::AIHistoryResponseToJsonValue(response);
}

memochat::json::JsonValue
AIServiceClient::CreateSession(int32_t uid, const std::string& model_type, const std::string& model_name)
{
    ai::AISessionRsp reply;
    auto status = _impl->makeCreateSessionCall(uid, model_type, model_name, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "AIServer unavailable"));
    }

    memochat::gate::services::ai::AISessionResponseDto response;
    response.code = reply.code();
    response.message = reply.message();
    if (reply.has_session())
    {
        response.session = SessionInfoToResponseDto(reply.session());
    }
    return memochat::gate::services::ai::AISessionResponseToJsonValue(response, reply.has_session());
}

memochat::json::JsonValue
AIServiceClient::ListSessions(int32_t uid, const std::string& model_type, const std::string& model_name)
{
    ai::AISessionRsp reply;
    auto status = _impl->makeListSessionsCall(uid, model_type, model_name, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "AIServer unavailable"));
    }

    memochat::gate::services::ai::AISessionListResponseDto response;
    response.code = reply.code();
    response.message = reply.message();
    for (const auto& s : reply.sessions())
    {
        response.sessions.push_back(SessionInfoToResponseDto(s));
    }
    return memochat::gate::services::ai::AISessionListResponseToJsonValue(response);
}

memochat::json::JsonValue AIServiceClient::DeleteSession(int32_t uid, const std::string& session_id)
{
    ai::AIDeleteSessionRsp reply;
    auto status = _impl->makeDeleteSessionCall(uid, session_id, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "AIServer unavailable"));
    }
    return memochat::gate::services::ai::AISimpleResponseToJsonValue(
        MakeSimpleResponse(reply.code(), reply.message()));
}

memochat::json::JsonValue
AIServiceClient::UpdateSession(int32_t uid, const std::string& session_id, const std::string& title)
{
    ai::AISessionRsp reply;
    auto status = _impl->makeUpdateSessionCall(uid, session_id, title, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "AIServer unavailable"));
    }

    memochat::gate::services::ai::AISessionResponseDto response;
    response.code = reply.code();
    response.message = reply.message();
    if (reply.has_session())
    {
        response.session = SessionInfoToResponseDto(reply.session());
    }
    return memochat::gate::services::ai::AISessionResponseToJsonValue(response, reply.has_session());
}

memochat::json::JsonValue AIServiceClient::ListModels()
{
    ai::AIListModelsRsp reply;
    auto status = _impl->makeListModelsCall(&reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "AIServer unavailable"));
    }

    memochat::gate::services::ai::AIModelListResponseDto response;
    response.code = reply.code();
    for (const auto& m : reply.models())
    {
        response.models.push_back(ModelInfoToResponseDto(m));
    }
    if (reply.has_default_model())
    {
        response.default_model = ModelInfoToResponseDto(reply.default_model());
    }
    return memochat::gate::services::ai::AIModelListResponseToJsonValue(response, reply.has_default_model());
}

memochat::json::JsonValue AIServiceClient::RegisterApiProvider(const std::string& provider_name,
                                                               const std::string& base_url,
                                                               const std::string& api_key,
                                                               const std::string& adapter)
{
    ai::AIRegisterApiProviderRsp reply;
    auto status = _impl->makeRegisterApiProviderCall(provider_name, base_url, api_key, adapter, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "AIServer unavailable"));
    }

    memochat::gate::services::ai::AIRegisterApiProviderResponseDto response;
    response.code = reply.code();
    response.message = reply.message();
    response.provider_id = reply.provider_id();
    for (const auto& m : reply.models())
    {
        response.models.push_back(ModelInfoToResponseDto(m));
    }
    return memochat::gate::services::ai::AIRegisterApiProviderResponseToJsonValue(response);
}

memochat::json::JsonValue AIServiceClient::DeleteApiProvider(const std::string& provider_id)
{
    ai::AIDeleteApiProviderRsp reply;
    auto status = _impl->makeDeleteApiProviderCall(provider_id, &reply);

    if (!status.ok())
    {
        memochat::gate::services::ai::AIDeleteApiProviderResponseDto response;
        response.code = 500;
        response.message = "AIServer unavailable";
        response.provider_id = provider_id;
        return memochat::gate::services::ai::AIDeleteApiProviderResponseToJsonValue(response);
    }

    memochat::gate::services::ai::AIDeleteApiProviderResponseDto response;
    response.code = reply.code();
    response.message = reply.message();
    response.provider_id = reply.provider_id();
    return memochat::gate::services::ai::AIDeleteApiProviderResponseToJsonValue(response);
}

memochat::json::JsonValue AIServiceClient::KbUpload(int32_t uid,
                                                    const std::string& file_name,
                                                    const std::string& file_type,
                                                    const std::string& base64_content)
{
    ai::AIKbUploadRsp reply;
    auto status = _impl->makeKbUploadCall(uid, file_name, file_type, base64_content, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(MakeSimpleResponse(500, "upload failed"));
    }

    memochat::gate::services::ai::AIKbUploadResponseDto response;
    response.code = reply.code();
    response.message = reply.message();
    response.chunks = reply.chunks();
    response.kb_id = reply.kb_id();
    return memochat::gate::services::ai::AIKbUploadResponseToJsonValue(response);
}

memochat::json::JsonValue AIServiceClient::KbSearch(int32_t uid, const std::string& query, int top_k)
{
    ai::AIKbSearchRsp reply;
    auto status = _impl->makeKbSearchCall(uid, query, top_k, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(MakeSimpleResponse(500, "search failed"));
    }

    memochat::gate::services::ai::AIKbSearchResponseDto response;
    response.code = reply.code();
    for (const auto& c : reply.chunks())
    {
        memochat::gate::services::ai::AIKbSearchChunkResponseDto chunk;
        chunk.content = c.content();
        chunk.score = c.score();
        chunk.source = c.source();
        response.chunks.push_back(std::move(chunk));
    }
    return memochat::gate::services::ai::AIKbSearchResponseToJsonValue(response);
}

memochat::json::JsonValue AIServiceClient::ListKb(int32_t uid)
{
    ai::AIKbListRsp reply;
    auto status = _impl->makeListKbCall(uid, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(MakeSimpleResponse(500, "list failed"));
    }

    memochat::gate::services::ai::AIKnowledgeBaseListResponseDto response;
    response.code = reply.code();
    for (const auto& kb : reply.knowledge_bases())
    {
        response.knowledge_bases.push_back(KnowledgeBaseToResponseDto(kb));
    }
    return memochat::gate::services::ai::AIKnowledgeBaseListResponseToJsonValue(response);
}

memochat::json::JsonValue AIServiceClient::DeleteKb(int32_t uid, const std::string& kb_id)
{
    ai::AIKbDeleteRsp reply;
    auto status = _impl->makeDeleteKbCall(uid, kb_id, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(MakeSimpleResponse(500, "delete failed"));
    }

    return memochat::gate::services::ai::AISimpleResponseToJsonValue(
        MakeSimpleResponse(reply.code(), reply.message()));
}

static memochat::json::JsonValue MemoryItemToJson(const ai::AIMemoryItem& item)
{
    memochat::gate::services::ai::AIMemoryItemResponseDto dto;
    dto.memory_id = item.memory_id();
    dto.type = item.type();
    dto.source = item.source();
    dto.content = item.content();
    dto.created_at = static_cast<double>(item.created_at());
    dto.updated_at = static_cast<double>(item.updated_at());
    return memochat::gate::services::ai::AIMemoryItemResponseToJsonValue(dto, item.metadata_json());
}

memochat::json::JsonValue AIServiceClient::MemoryList(int32_t uid)
{
    ai::AIMemoryListRsp reply;
    auto status = _impl->makeMemoryListCall(uid, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "memory list failed"));
    }

    memochat::json::JsonValue root = memochat::gate::services::ai::AISimpleResponseToJsonValue(
        MakeSimpleResponse(reply.code(), reply.message()));
    array_t memories;
    for (const auto& memory : reply.memories())
    {
        memories.push_back(MemoryItemToJson(memory).impl());
    }
    root["memories"] = std::move(memories);
    return root;
}

memochat::json::JsonValue AIServiceClient::MemoryCreate(int32_t uid, const std::string& content)
{
    ai::AIMemoryRsp reply;
    auto status = _impl->makeMemoryCreateCall(uid, content, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "memory create failed"));
    }

    memochat::json::JsonValue root = memochat::gate::services::ai::AISimpleResponseToJsonValue(
        MakeSimpleResponse(reply.code(), reply.message()));
    if (reply.has_memory())
    {
        root["memory"] = MemoryItemToJson(reply.memory());
    }
    return root;
}

memochat::json::JsonValue AIServiceClient::MemoryDelete(int32_t uid, const std::string& memory_id)
{
    ai::AIMemoryRsp reply;
    auto status = _impl->makeMemoryDeleteCall(uid, memory_id, &reply);

    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "memory delete failed"));
    }
    return memochat::gate::services::ai::AISimpleResponseToJsonValue(
        MakeSimpleResponse(reply.code(), reply.message()));
}

static memochat::json::JsonValue AgentTaskItemToJson(const ai::AIAgentTaskItem& item)
{
    memochat::gate::services::ai::AIAgentTaskItemResponseDto dto;
    dto.task_id = item.task_id();
    dto.title = item.title();
    dto.status = item.status();
    dto.trace_id = item.trace_id();
    dto.description = item.description();
    dto.priority = item.priority();
    dto.error = item.error();
    dto.created_at = static_cast<double>(item.created_at());
    dto.updated_at = static_cast<double>(item.updated_at());
    dto.completed_at = static_cast<double>(item.completed_at());
    dto.cancelled_at = static_cast<double>(item.cancelled_at());
    return memochat::gate::services::ai::AIAgentTaskItemResponseToJsonValue(
        dto, item.payload_json(), item.result_json(), item.checkpoints_json(), item.metadata_json());
}

static memochat::json::JsonValue AgentTaskRspToJson(const ai::AIAgentTaskRsp& reply)
{
    memochat::json::JsonValue root = memochat::gate::services::ai::AISimpleResponseToJsonValue(
        memochat::gate::services::ai::AISimpleResponseDto{reply.code(), reply.message()});
    if (reply.has_task())
    {
        root["task"] = AgentTaskItemToJson(reply.task());
    }
    array_t tasks;
    for (const auto& task : reply.tasks())
    {
        tasks.push_back(AgentTaskItemToJson(task).impl());
    }
    root["tasks"] = std::move(tasks);
    return root;
}

memochat::json::JsonValue AIServiceClient::AgentTaskCreate(int32_t uid,
                                                           const std::string& title,
                                                           const std::string& content,
                                                           const std::string& session_id,
                                                           const std::string& model_type,
                                                           const std::string& model_name,
                                                           const std::string& skill_name,
                                                           const std::string& metadata_json)
{
    ai::AIAgentTaskRsp reply;
    auto status = _impl->makeAgentTaskCreateCall(uid,
                                                 title,
                                                 content,
                                                 session_id,
                                                 model_type,
                                                 model_name,
                                                 skill_name,
                                                 metadata_json,
                                                 &reply);
    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "task create failed"));
    }
    return AgentTaskRspToJson(reply);
}

memochat::json::JsonValue AIServiceClient::AgentTaskList(int32_t uid, int limit)
{
    ai::AIAgentTaskRsp reply;
    auto status = _impl->makeAgentTaskListCall(uid, limit, &reply);
    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "task list failed"));
    }
    return AgentTaskRspToJson(reply);
}

memochat::json::JsonValue AIServiceClient::AgentTaskGet(const std::string& task_id)
{
    ai::AIAgentTaskRsp reply;
    auto status = _impl->makeAgentTaskGetCall(task_id, &reply);
    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "task get failed"));
    }
    return AgentTaskRspToJson(reply);
}

memochat::json::JsonValue AIServiceClient::AgentTaskCancel(const std::string& task_id)
{
    ai::AIAgentTaskRsp reply;
    auto status = _impl->makeAgentTaskCancelCall(task_id, &reply);
    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "task cancel failed"));
    }
    return AgentTaskRspToJson(reply);
}

memochat::json::JsonValue AIServiceClient::AgentTaskResume(const std::string& task_id)
{
    ai::AIAgentTaskRsp reply;
    auto status = _impl->makeAgentTaskResumeCall(task_id, &reply);
    if (!status.ok())
    {
        return memochat::gate::services::ai::AISimpleResponseToJsonValue(
            MakeSimpleResponse(500, "task resume failed"));
    }
    return AgentTaskRspToJson(reply);
}

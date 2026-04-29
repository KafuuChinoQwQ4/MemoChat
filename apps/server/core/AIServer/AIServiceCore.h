#pragma once
#include <grpcpp/grpcpp.h>
#include "common/proto/ai_message.grpc.pb.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

class AIServiceClient;
class AISessionRepo;
class ConversationContext;

class AIServiceCore {
public:
    explicit AIServiceCore();
    ~AIServiceCore();

    // 对话处理
    grpc::Status HandleChat(const ai::AIChatReq& req, ai::AIChatRsp* reply);
    grpc::Status HandleChatStream(const ai::AIChatReq& req,
                                  grpc::ServerWriter<ai::AIChatStreamChunk>* writer);

    // 智能功能
    grpc::Status HandleSmart(const ai::AISmartReq& req, ai::AISmartRsp* reply);

    // 历史
    grpc::Status GetHistory(const ai::AIHistoryReq& req, ai::AIHistoryRsp* reply);

    // 会话管理
    grpc::Status CreateSession(const ai::AICreateSessionReq& req, ai::AISessionRsp* reply);
    grpc::Status ListSessions(const ai::AICreateSessionReq& req, ai::AISessionRsp* reply);
    grpc::Status DeleteSession(const ai::AIDeleteSessionReq& req, ai::AIDeleteSessionRsp* reply);

    // 模型
    grpc::Status ListModels(const ai::AIListModelsReq& req, ai::AIListModelsRsp* reply);
    grpc::Status RegisterApiProvider(const ai::AIRegisterApiProviderReq& req, ai::AIRegisterApiProviderRsp* reply);

    // 知识库
    grpc::Status HandleKbUpload(const ai::AIKbUploadReq& req, ai::AIKbUploadRsp* reply);
    grpc::Status HandleKbSearch(const ai::AIKbSearchReq& req, ai::AIKbSearchRsp* reply);
    grpc::Status ListKb(const ai::AIKbListReq& req, ai::AIKbListRsp* reply);
    grpc::Status DeleteKb(const ai::AIKbDeleteReq& req, ai::AIKbDeleteRsp* reply);

    // 人类确认
    grpc::Status HandleConfirm(const ai::AIConfirmReq& req, ai::AIConfirmRsp* reply);

private:
    std::string GetOrCreateSessionId(int32_t uid, const std::string& model_type,
                                       const std::string& model_name);
    void SaveUserMessage(const std::string& session_id, int32_t uid,
                         const std::string& content, const std::string& model_name);
    void SaveAIMessage(const std::string& session_id, int32_t uid,
                        const std::string& content, const std::string& model_name,
                        int64_t tokens_used);

    std::unique_ptr<AIServiceClient> _ai_client;
    std::unique_ptr<AISessionRepo> _session_repo;

    std::mutex _session_cache_mtx;
    std::unordered_map<std::string, std::shared_ptr<ConversationContext>> _session_cache;
};

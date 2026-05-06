#pragma once

#include <grpcpp/grpcpp.h>
#include "common/proto/ai_message.grpc.pb.h"

#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>

using grpc::ServerContext;

class AIServiceCore;

class AIServiceImpl final : public ai::AIService::Service {
public:
    explicit AIServiceImpl();
    ~AIServiceImpl() override;

    grpc::Status Chat(ServerContext* context,
                      const ai::AIChatReq* request,
                      ai::AIChatRsp* reply) override;

    grpc::Status ChatStream(ServerContext* context,
                           const ai::AIChatStreamReq* request,
                           grpc::ServerWriter<ai::AIChatStreamChunk>* writer) override;

    grpc::Status Smart(ServerContext* context,
                       const ai::AISmartReq* request,
                       ai::AISmartRsp* reply) override;

    grpc::Status GetHistory(ServerContext* context,
                            const ai::AIHistoryReq* request,
                            ai::AIHistoryRsp* reply) override;

    grpc::Status CreateSession(ServerContext* context,
                               const ai::AICreateSessionReq* request,
                               ai::AISessionRsp* reply) override;

    grpc::Status ListSessions(ServerContext* context,
                              const ai::AICreateSessionReq* request,
                              ai::AISessionRsp* reply) override;

    grpc::Status DeleteSession(ServerContext* context,
                              const ai::AIDeleteSessionReq* request,
                              ai::AIDeleteSessionRsp* reply) override;

    grpc::Status ListModels(ServerContext* context,
                            const ai::AIListModelsReq* request,
                            ai::AIListModelsRsp* reply) override;

    grpc::Status RegisterApiProvider(ServerContext* context,
                                     const ai::AIRegisterApiProviderReq* request,
                                     ai::AIRegisterApiProviderRsp* reply) override;

    grpc::Status DeleteApiProvider(ServerContext* context,
                                   const ai::AIDeleteApiProviderReq* request,
                                   ai::AIDeleteApiProviderRsp* reply) override;

    grpc::Status KbUpload(ServerContext* context,
                           const ai::AIKbUploadReq* request,
                           ai::AIKbUploadRsp* reply) override;

    grpc::Status KbSearch(ServerContext* context,
                          const ai::AIKbSearchReq* request,
                          ai::AIKbSearchRsp* reply) override;

    grpc::Status KbList(ServerContext* context,
                         const ai::AIKbListReq* request,
                         ai::AIKbListRsp* reply) override;

    grpc::Status KbDelete(ServerContext* context,
                           const ai::AIKbDeleteReq* request,
                           ai::AIKbDeleteRsp* reply) override;

    grpc::Status MemoryList(ServerContext* context,
                            const ai::AIMemoryReq* request,
                            ai::AIMemoryListRsp* reply) override;

    grpc::Status MemoryCreate(ServerContext* context,
                              const ai::AIMemoryReq* request,
                              ai::AIMemoryRsp* reply) override;

    grpc::Status MemoryDelete(ServerContext* context,
                              const ai::AIMemoryReq* request,
                              ai::AIMemoryRsp* reply) override;

    grpc::Status AgentTaskCreate(ServerContext* context,
                                 const ai::AIAgentTaskReq* request,
                                 ai::AIAgentTaskRsp* reply) override;

    grpc::Status AgentTaskList(ServerContext* context,
                               const ai::AIAgentTaskReq* request,
                               ai::AIAgentTaskRsp* reply) override;

    grpc::Status AgentTaskGet(ServerContext* context,
                              const ai::AIAgentTaskReq* request,
                              ai::AIAgentTaskRsp* reply) override;

    grpc::Status AgentTaskCancel(ServerContext* context,
                                 const ai::AIAgentTaskReq* request,
                                 ai::AIAgentTaskRsp* reply) override;

    grpc::Status AgentTaskResume(ServerContext* context,
                                 const ai::AIAgentTaskReq* request,
                                 ai::AIAgentTaskRsp* reply) override;

    grpc::Status Confirm(ServerContext* context,
                         const ai::AIConfirmReq* request,
                         ai::AIConfirmRsp* reply) override;

private:
    std::unique_ptr<AIServiceCore> _core;
};

#include "AIServiceImpl.h"
#include "AIServiceCore.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"

AIServiceImpl::AIServiceImpl()
    : _core(std::make_unique<AIServiceCore>()) {
}

AIServiceImpl::~AIServiceImpl() = default;

grpc::Status AIServiceImpl::Chat(ServerContext* context,
                                 const ai::AIChatReq* request,
                                 ai::AIChatRsp* reply) {
    memolog::SpanScope span("AIService.Chat", "gRPC");
    return _core->HandleChat(*request, reply);
}

grpc::Status AIServiceImpl::ChatStream(ServerContext* context,
                                     const ai::AIChatStreamReq* request,
                                     grpc::ServerWriter<ai::AIChatStreamChunk>* writer) {
    memolog::SpanScope span("AIService.ChatStream", "gRPC");
    return _core->HandleChatStream(request->req(), writer);
}

grpc::Status AIServiceImpl::Smart(ServerContext* context,
                                 const ai::AISmartReq* request,
                                 ai::AISmartRsp* reply) {
    memolog::SpanScope span("AIService.Smart", "gRPC");
    return _core->HandleSmart(*request, reply);
}

grpc::Status AIServiceImpl::GetHistory(ServerContext* context,
                                     const ai::AIHistoryReq* request,
                                     ai::AIHistoryRsp* reply) {
    memolog::SpanScope span("AIService.GetHistory", "gRPC");
    return _core->GetHistory(*request, reply);
}

grpc::Status AIServiceImpl::CreateSession(ServerContext* context,
                                       const ai::AICreateSessionReq* request,
                                       ai::AISessionRsp* reply) {
    memolog::SpanScope span("AIService.CreateSession", "gRPC");
    return _core->CreateSession(*request, reply);
}

grpc::Status AIServiceImpl::ListSessions(ServerContext* context,
                                        const ai::AICreateSessionReq* request,
                                        ai::AISessionRsp* reply) {
    memolog::SpanScope span("AIService.ListSessions", "gRPC");
    return _core->ListSessions(*request, reply);
}

grpc::Status AIServiceImpl::DeleteSession(ServerContext* context,
                                        const ai::AIDeleteSessionReq* request,
                                        ai::AIDeleteSessionRsp* reply) {
    memolog::SpanScope span("AIService.DeleteSession", "gRPC");
    return _core->DeleteSession(*request, reply);
}

grpc::Status AIServiceImpl::ListModels(ServerContext* context,
                                      const ai::AIListModelsReq* request,
                                      ai::AIListModelsRsp* reply) {
    memolog::SpanScope span("AIService.ListModels", "gRPC");
    return _core->ListModels(*request, reply);
}

grpc::Status AIServiceImpl::KbUpload(ServerContext* context,
                                    const ai::AIKbUploadReq* request,
                                    ai::AIKbUploadRsp* reply) {
    memolog::SpanScope span("AIService.KbUpload", "gRPC");
    return _core->HandleKbUpload(*request, reply);
}

grpc::Status AIServiceImpl::KbSearch(ServerContext* context,
                                    const ai::AIKbSearchReq* request,
                                    ai::AIKbSearchRsp* reply) {
    memolog::SpanScope span("AIService.KbSearch", "gRPC");
    return _core->HandleKbSearch(*request, reply);
}

grpc::Status AIServiceImpl::KbList(ServerContext* context,
                                   const ai::AIKbListReq* request,
                                   ai::AIKbListRsp* reply) {
    memolog::SpanScope span("AIService.KbList", "gRPC");
    return _core->ListKb(*request, reply);
}

grpc::Status AIServiceImpl::KbDelete(ServerContext* context,
                                    const ai::AIKbDeleteReq* request,
                                    ai::AIKbDeleteRsp* reply) {
    memolog::SpanScope span("AIService.KbDelete", "gRPC");
    return _core->DeleteKb(*request, reply);
}

grpc::Status AIServiceImpl::Confirm(ServerContext* context,
                                   const ai::AIConfirmReq* request,
                                   ai::AIConfirmRsp* reply) {
    memolog::SpanScope span("AIService.Confirm", "gRPC");
    return _core->HandleConfirm(*request, reply);
}

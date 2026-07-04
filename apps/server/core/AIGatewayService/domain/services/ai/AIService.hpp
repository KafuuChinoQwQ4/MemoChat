#pragma once

#include "routing/GateRequest.hpp"
#include "routing/GateResponse.hpp"

namespace memochat::gate::services::ai
{

class AIService
{
public:
    static AIService& Instance();

    bool HandleChat(const memochat::gate::routing::GateRequest& request,
                    memochat::gate::routing::GateResponse& response);
    bool HandleSmart(const memochat::gate::routing::GateRequest& request,
                     memochat::gate::routing::GateResponse& response);
    bool HandleHistory(const memochat::gate::routing::GateRequest& request,
                       memochat::gate::routing::GateResponse& response);
    bool HandleCreateSession(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response);
    bool HandleListSessions(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleDeleteSession(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response);
    bool HandleUpdateSession(const memochat::gate::routing::GateRequest& request,
                             memochat::gate::routing::GateResponse& response);
    bool HandleListModels(const memochat::gate::routing::GateRequest& request,
                          memochat::gate::routing::GateResponse& response);
    bool HandleRegisterApiProvider(const memochat::gate::routing::GateRequest& request,
                                   memochat::gate::routing::GateResponse& response);
    bool HandleDeleteApiProvider(const memochat::gate::routing::GateRequest& request,
                                 memochat::gate::routing::GateResponse& response);
    bool HandleKbUpload(const memochat::gate::routing::GateRequest& request,
                        memochat::gate::routing::GateResponse& response);
    bool HandleKbSearch(const memochat::gate::routing::GateRequest& request,
                        memochat::gate::routing::GateResponse& response);
    bool HandleListKb(const memochat::gate::routing::GateRequest& request,
                      memochat::gate::routing::GateResponse& response);
    bool HandleDeleteKb(const memochat::gate::routing::GateRequest& request,
                        memochat::gate::routing::GateResponse& response);
    bool HandleMemoryList(const memochat::gate::routing::GateRequest& request,
                          memochat::gate::routing::GateResponse& response);
    bool HandleMemoryCreate(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleMemoryDelete(const memochat::gate::routing::GateRequest& request,
                            memochat::gate::routing::GateResponse& response);
    bool HandleTaskCreate(const memochat::gate::routing::GateRequest& request,
                          memochat::gate::routing::GateResponse& response);
    bool HandleTaskList(const memochat::gate::routing::GateRequest& request,
                        memochat::gate::routing::GateResponse& response);
    bool HandleTaskDetail(const memochat::gate::routing::GateRequest& request,
                          memochat::gate::routing::GateResponse& response);
    bool HandleTaskCancel(const memochat::gate::routing::GateRequest& request,
                          memochat::gate::routing::GateResponse& response);
    bool HandleTaskResume(const memochat::gate::routing::GateRequest& request,
                          memochat::gate::routing::GateResponse& response);

private:
    AIService() = default;
};

} // namespace memochat::gate::services::ai

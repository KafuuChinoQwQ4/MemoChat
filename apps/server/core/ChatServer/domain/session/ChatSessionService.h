#pragma once

#include "ports/IChatSessionConfig.h"
#include "ports/IChatSessionRepository.h"
#include "ports/IOnlineRouteStore.h"
#include "ports/IRelationQueryService.h"
#include "ports/ISessionRegistry.h"

#include <memory>
#include <string>

class CSession;
class LogicSystem;

class ChatSessionService
{
public:
    ChatSessionService(LogicSystem& logic,
                       ISessionRegistry* session_registry,
                       IOnlineRouteStore* online_route_store,
                       IRelationQueryService* relation_query_service,
                       IChatSessionConfig* session_config,
                       IChatSessionRepository* session_repository);

    void HandleLogin(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleRelationBootstrap(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleHeartbeat(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);

private:
    void PushOfflineMessages(const std::shared_ptr<CSession>& session, int uid);
    LogicSystem& _logic;
    ISessionRegistry* _session_registry = nullptr;
    IOnlineRouteStore* _online_route_store = nullptr;
    IRelationQueryService* _relation_query_service = nullptr;
    IChatSessionConfig* _session_config = nullptr;
    IChatSessionRepository* _session_repository = nullptr;
};

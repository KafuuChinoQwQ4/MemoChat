#pragma once

#include "ports/IChatSessionConfig.hpp"
#include "ports/IChatSessionRepository.hpp"
#include "ports/IOnlineRouteStore.hpp"
#include "ports/IRelationQueryService.hpp"
#include "ports/IRelationRepository.hpp"
#include "ports/ISessionRegistry.hpp"

#include <memory>
#include <string>

class IChatSession;
class LogicSystem;

class ChatSessionService
{
public:
    ChatSessionService(LogicSystem& logic,
                       ISessionRegistry* session_registry,
                       IOnlineRouteStore* online_route_store,
                       IRelationQueryService* relation_query_service,
                       IRelationRepository* relation_repository,
                       IChatSessionConfig* session_config,
                       IChatSessionRepository* session_repository);

    void HandleLogin(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data);
    void
    HandleRelationBootstrap(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data);
    void HandleHeartbeat(const std::shared_ptr<IChatSession>& session, short msg_id, const std::string& msg_data);

private:
    void PushOfflineMessages(const std::shared_ptr<IChatSession>& session, int uid);
    LogicSystem& _logic;
    ISessionRegistry* _session_registry = nullptr;
    IOnlineRouteStore* _online_route_store = nullptr;
    IRelationQueryService* _relation_query_service = nullptr;
    IRelationRepository* _relation_repository = nullptr;
    IChatSessionConfig* _session_config = nullptr;
    IChatSessionRepository* _session_repository = nullptr;
};

#include "modules/ai/AIRouteModule.h"

#include "routing/RouteRegistry.h"
#include "services/ai/AIService.h"

namespace memochat::gate::modules::ai
{

void AIRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register(
        "POST",
        "/ai/chat",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleChat(request, response);
        });
    registry.Register(
        "POST",
        "/ai/smart",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleSmart(request, response);
        });
    registry.Register(
        "GET",
        "/ai/history",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleHistory(request, response);
        });
    registry.Register(
        "POST",
        "/ai/session",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleCreateSession(request, response);
        });
    registry.Register(
        "GET",
        "/ai/session/list",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleListSessions(request, response);
        });
    registry.Register(
        "POST",
        "/ai/session/delete",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleDeleteSession(request, response);
        });
    registry.Register(
        "POST",
        "/ai/session/update",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleUpdateSession(request, response);
        });
    registry.Register(
        "GET",
        "/ai/model/list",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleListModels(request, response);
        });
    registry.Register(
        "POST",
        "/ai/model/api/register",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleRegisterApiProvider(request, response);
        });
    registry.Register(
        "POST",
        "/ai/model/api/delete",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleDeleteApiProvider(request, response);
        });
    registry.Register(
        "POST",
        "/ai/kb/upload",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleKbUpload(request, response);
        });
    registry.Register(
        "POST",
        "/ai/kb/search",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleKbSearch(request, response);
        });
    registry.Register(
        "GET",
        "/ai/kb/list",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleListKb(request, response);
        });
    registry.Register(
        "POST",
        "/ai/kb/delete",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleDeleteKb(request, response);
        });
    registry.Register(
        "GET",
        "/ai/memory/list",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleMemoryList(request, response);
        });
    registry.Register(
        "POST",
        "/ai/memory",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleMemoryCreate(request, response);
        });
    registry.Register(
        "POST",
        "/ai/memory/delete",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleMemoryDelete(request, response);
        });
    registry.Register(
        "POST",
        "/ai/tasks",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskCreate(request, response);
        });
    registry.Register(
        "GET",
        "/ai/tasks",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskList(request, response);
        });
    registry.Register(
        "GET",
        "/ai/tasks/detail",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskDetail(request, response);
        });
    registry.Register(
        "POST",
        "/ai/tasks/cancel",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskCancel(request, response);
        });
    registry.Register(
        "POST",
        "/ai/tasks/resume",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskResume(request, response);
        });
}

} // namespace memochat::gate::modules::ai

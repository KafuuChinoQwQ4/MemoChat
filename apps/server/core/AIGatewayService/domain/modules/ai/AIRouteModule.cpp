#include "modules/ai/AIRouteModule.hpp"

#include "routing/RouteRegistry.hpp"
#include "services/ai/AIService.hpp"

import memochat.ai.route_registration_algorithms;

namespace memochat::gate::modules::ai
{

void AIRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    namespace modules = memochat::ai::route_registration::modules;

    registry.Register(
        modules::PostMethod(),
        modules::ChatPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleChat(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::SmartPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleSmart(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::HistoryPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleHistory(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::SessionPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleCreateSession(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::SessionListPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleListSessions(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::SessionDeletePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleDeleteSession(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::SessionUpdatePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleUpdateSession(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::ModelListPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleListModels(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::ModelApiRegisterPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleRegisterApiProvider(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::ModelApiDeletePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleDeleteApiProvider(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::KbUploadPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleKbUpload(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::KbSearchPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleKbSearch(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::KbListPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleListKb(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::KbDeletePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleDeleteKb(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::MemoryListPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleMemoryList(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::MemoryPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleMemoryCreate(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::MemoryDeletePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleMemoryDelete(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::TasksPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskCreate(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::TasksPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskList(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::TaskDetailPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskDetail(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::TaskCancelPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskCancel(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::TaskResumePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::ai::AIService::Instance().HandleTaskResume(request, response);
        });
}

} // namespace memochat::gate::modules::ai

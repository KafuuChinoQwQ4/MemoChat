#include "modules/moments/MomentsRouteModule.h"

#include "routing/RouteRegistry.h"
#include "services/moments/MomentsService.h"

namespace memochat::gate::modules::moments
{

void MomentsRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register(
        "POST",
        "/api/moments/publish",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::moments::MomentsService::Instance().HandlePublish(request, response);
        });
    registry.Register(
        "POST",
        "/api/moments/list",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::moments::MomentsService::Instance().HandleList(request, response);
        });
    registry.Register(
        "POST",
        "/api/moments/detail",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::moments::MomentsService::Instance().HandleDetail(request, response);
        });
    registry.Register(
        "POST",
        "/api/moments/delete",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::moments::MomentsService::Instance().HandleDelete(request, response);
        });
    registry.Register(
        "POST",
        "/api/moments/like",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::moments::MomentsService::Instance().HandleLike(request, response);
        });
    registry.Register(
        "POST",
        "/api/moments/comment",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::moments::MomentsService::Instance().HandleComment(request, response);
        });
    registry.Register(
        "POST",
        "/api/moments/comment/list",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::moments::MomentsService::Instance().HandleCommentList(request, response);
        });
    registry.Register(
        "POST",
        "/api/moments/comment/like",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::moments::MomentsService::Instance().HandleCommentLike(request, response);
        });
}

} // namespace memochat::gate::modules::moments

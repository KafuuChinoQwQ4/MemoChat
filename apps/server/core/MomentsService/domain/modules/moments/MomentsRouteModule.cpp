#include "modules/moments/MomentsRouteModule.hpp"

#include "routing/RouteRegistry.hpp"
#include "services/moments/MomentsService.hpp"

import memochat.moments.route_registration_algorithms;

namespace memochat::gate::modules::moments
{

void MomentsRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    namespace modules = memochat::moments::route_registration::modules;
    // Single Instance() call at registration time; stable pointer captured by all lambdas.
    auto* svc = &memochat::gate::services::moments::MomentsService::Instance();

    registry.Register(
        modules::PostMethod(),
        modules::PublishPath(),
        [svc](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return svc->HandlePublish(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::ListPath(),
        [svc](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return svc->HandleList(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::DetailPath(),
        [svc](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return svc->HandleDetail(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::DeletePath(),
        [svc](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return svc->HandleDelete(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::LikePath(),
        [svc](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return svc->HandleLike(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::CommentPath(),
        [svc](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return svc->HandleComment(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::CommentListPath(),
        [svc](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return svc->HandleCommentList(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::CommentLikePath(),
        [svc](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return svc->HandleCommentLike(request, response);
        });
}

} // namespace memochat::gate::modules::moments

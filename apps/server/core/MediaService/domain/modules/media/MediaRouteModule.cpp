#include "modules/media/MediaRouteModule.hpp"

#include "routing/RouteRegistry.hpp"
#include "services/media/MediaService.hpp"

import memochat.media.route_registration_algorithms;

namespace memochat::gate::modules::media
{

void MediaRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    namespace modules = memochat::media::route_registration::modules;

    registry.Register(
        modules::PostMethod(),
        modules::UploadInitPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaInit(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::UploadChunkPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaChunk(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::UploadStatusPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaStatus(request, response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::UploadCompletePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaComplete(request,
                                                                                                       response);
        });
    registry.Register(
        modules::PostMethod(),
        modules::UploadSimplePath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaSimple(request, response);
        });
    registry.Register(
        modules::GetMethod(),
        modules::MediaDownloadPath(),
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleMediaDownload(request, response);
        });
}

} // namespace memochat::gate::modules::media

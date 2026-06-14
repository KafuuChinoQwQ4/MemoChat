#include "modules/media/MediaRouteModule.h"

#include "routing/RouteRegistry.h"
#include "services/media/MediaService.h"

namespace memochat::gate::modules::media
{

void MediaRouteModule::RegisterRoutes(memochat::gate::routing::RouteRegistry& registry)
{
    registry.Register(
        "POST",
        "/upload_media_init",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaInit(request, response);
        });
    registry.Register(
        "POST",
        "/upload_media_chunk",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaChunk(request, response);
        });
    registry.Register(
        "GET",
        "/upload_media_status",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaStatus(request, response);
        });
    registry.Register(
        "POST",
        "/upload_media_complete",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaComplete(request,
                                                                                                       response);
        });
    registry.Register(
        "POST",
        "/upload_media",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleUploadMediaSimple(request, response);
        });
    registry.Register(
        "GET",
        "/media/download",
        [](const memochat::gate::routing::GateRequest& request, memochat::gate::routing::GateResponse& response)
        {
            return memochat::gate::services::media::MediaService::Instance().HandleMediaDownload(request, response);
        });
}

} // namespace memochat::gate::modules::media

#include "modules/media/MediaRouteModule.h"

#include "services/media/MediaPublicDtos.h"

namespace memochat::gate::modules::media
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> MediaRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<memochat::media::MediaUploadInitRequestDto, memochat::media::MediaUploadInitResponseDto>(
            "POST",
            "/upload_media_init",
            "media.upload.init",
            "MediaUploadInitRequestDto",
            "MediaUploadInitResponseDto"),
        MakeRouteSchema<memochat::media::MediaUploadChunkJsonRequestDto, memochat::media::MediaUploadChunkResponseDto>(
            "POST",
            "/upload_media_chunk",
            "media.upload.chunk_json",
            "MediaUploadChunkJsonRequestDto",
            "MediaUploadChunkResponseDto"),
        MakeRouteSchema<memochat::media::MediaUploadCompleteRequestDto, memochat::media::MediaUploadAssetResponseDto>(
            "POST",
            "/upload_media_complete",
            "media.upload.complete",
            "MediaUploadCompleteRequestDto",
            "MediaUploadAssetResponseDto"),
        MakeRouteSchema<memochat::media::MediaUploadSimpleRequestDto, memochat::media::MediaUploadAssetResponseDto>(
            "POST",
            "/upload_media",
            "media.upload.simple",
            "MediaUploadSimpleRequestDto",
            "MediaUploadAssetResponseDto"),
    };
}

} // namespace memochat::gate::modules::media

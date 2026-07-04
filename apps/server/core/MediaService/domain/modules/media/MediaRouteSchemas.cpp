#include "modules/media/MediaRouteModule.hpp"

#include "services/media/MediaPublicDtos.hpp"

import memochat.media.route_schema_algorithms;

namespace memochat::gate::modules::media
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> MediaRouteModule::RouteSchemas()
{
    using memochat::gate::routing::MakeRouteSchema;
    namespace modules = memochat::media::route_schema::modules;

    return {
        MakeRouteSchema<memochat::media::MediaUploadInitRequestDto, memochat::media::MediaUploadInitResponseDto>(
            modules::PostMethod(),
            modules::UploadInitPath(),
            modules::UploadInitRouteName(),
            modules::UploadInitRequestTypeName(),
            modules::UploadInitResponseTypeName()),
        MakeRouteSchema<memochat::media::MediaUploadChunkJsonRequestDto, memochat::media::MediaUploadChunkResponseDto>(
            modules::PostMethod(),
            modules::UploadChunkJsonPath(),
            modules::UploadChunkJsonRouteName(),
            modules::UploadChunkJsonRequestTypeName(),
            modules::UploadChunkJsonResponseTypeName()),
        MakeRouteSchema<memochat::media::MediaUploadCompleteRequestDto, memochat::media::MediaUploadAssetResponseDto>(
            modules::PostMethod(),
            modules::UploadCompletePath(),
            modules::UploadCompleteRouteName(),
            modules::UploadCompleteRequestTypeName(),
            modules::UploadAssetResponseTypeName()),
        MakeRouteSchema<memochat::media::MediaUploadSimpleRequestDto, memochat::media::MediaUploadAssetResponseDto>(
            modules::PostMethod(),
            modules::UploadSimplePath(),
            modules::UploadSimpleRouteName(),
            modules::UploadSimpleRequestTypeName(),
            modules::UploadAssetResponseTypeName()),
    };
}

} // namespace memochat::gate::modules::media

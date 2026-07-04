#include "modules/moments/MomentsRouteModule.hpp"

#include "services/moments/MomentsOutputDtos.hpp"
#include "services/moments/MomentsPublicDtos.hpp"

import memochat.moments.route_schema_algorithms;

namespace memochat::gate::modules::moments
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> MomentsRouteModule::RouteSchemas()
{
    namespace svc = memochat::gate::services::moments;
    namespace modules = memochat::moments::route_schema::modules;
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<svc::MomentPublishRequestDto, svc::MomentIdResponseDto>(modules::PostMethod(),
                                                                                modules::PublishPath(),
                                                                                modules::PublishRouteName(),
                                                                                modules::PublishRequestTypeName(),
                                                                                modules::MomentIdResponseTypeName()),
        MakeRouteSchema<svc::MomentListRequestDto, svc::MomentListResponseDto>(modules::PostMethod(),
                                                                               modules::ListPath(),
                                                                               modules::ListRouteName(),
                                                                               modules::ListRequestTypeName(),
                                                                               modules::ListResponseTypeName()),
        MakeRouteSchema<svc::MomentIdRequestDto, svc::MomentDetailResponseDto>(modules::PostMethod(),
                                                                               modules::DetailPath(),
                                                                               modules::DetailRouteName(),
                                                                               modules::MomentIdRequestTypeName(),
                                                                               modules::DetailResponseTypeName()),
        MakeRouteSchema<svc::MomentIdRequestDto, svc::MomentIdResponseDto>(modules::PostMethod(),
                                                                           modules::DeletePath(),
                                                                           modules::DeleteRouteName(),
                                                                           modules::MomentIdRequestTypeName(),
                                                                           modules::MomentIdResponseTypeName()),
        MakeRouteSchema<svc::MomentLikeRequestDto, svc::MomentLikeResponseDto>(modules::PostMethod(),
                                                                               modules::LikePath(),
                                                                               modules::LikeRouteName(),
                                                                               modules::LikeRequestTypeName(),
                                                                               modules::LikeResponseTypeName()),
        MakeRouteSchema<svc::MomentCommentRequestDto, svc::MomentCommentMutationResponseDto>(
            modules::PostMethod(),
            modules::CommentPath(),
            modules::CommentRouteName(),
            modules::CommentRequestTypeName(),
            modules::CommentMutationResponseTypeName(),
            {{"delete_", "delete"}},
            {{"delete_", "delete"}}),
        MakeRouteSchema<svc::MomentCommentListRequestDto, svc::MomentCommentListResponseDto>(
            modules::PostMethod(),
            modules::CommentListPath(),
            modules::CommentListRouteName(),
            modules::CommentListRequestTypeName(),
            modules::CommentListResponseTypeName()),
        MakeRouteSchema<svc::MomentCommentLikeRequestDto, svc::MomentCommentLikeResponseDto>(
            modules::PostMethod(),
            modules::CommentLikePath(),
            modules::CommentLikeRouteName(),
            modules::CommentLikeRequestTypeName(),
            modules::CommentLikeResponseTypeName()),
    };
}

} // namespace memochat::gate::modules::moments

#include "modules/moments/MomentsRouteModule.h"

#include "services/moments/MomentsOutputDtos.h"
#include "services/moments/MomentsPublicDtos.h"

namespace memochat::gate::modules::moments
{

std::vector<memochat::gate::routing::RouteSchemaDescriptor> MomentsRouteModule::RouteSchemas()
{
    namespace svc = memochat::gate::services::moments;
    using memochat::gate::routing::MakeRouteSchema;

    return {
        MakeRouteSchema<svc::MomentPublishRequestDto, svc::MomentIdResponseDto>(
            "POST", "/api/moments/publish", "moments.publish", "MomentPublishRequestDto", "MomentIdResponseDto"),
        MakeRouteSchema<svc::MomentListRequestDto, svc::MomentListResponseDto>(
            "POST", "/api/moments/list", "moments.list", "MomentListRequestDto", "MomentListResponseDto"),
        MakeRouteSchema<svc::MomentIdRequestDto, svc::MomentDetailResponseDto>(
            "POST", "/api/moments/detail", "moments.detail", "MomentIdRequestDto", "MomentDetailResponseDto"),
        MakeRouteSchema<svc::MomentIdRequestDto, svc::MomentIdResponseDto>(
            "POST", "/api/moments/delete", "moments.delete", "MomentIdRequestDto", "MomentIdResponseDto"),
        MakeRouteSchema<svc::MomentLikeRequestDto, svc::MomentLikeResponseDto>(
            "POST", "/api/moments/like", "moments.like", "MomentLikeRequestDto", "MomentLikeResponseDto"),
        MakeRouteSchema<svc::MomentCommentRequestDto, svc::MomentCommentMutationResponseDto>(
            "POST",
            "/api/moments/comment",
            "moments.comment",
            "MomentCommentRequestDto",
            "MomentCommentMutationResponseDto",
            {{"delete_", "delete"}},
            {{"delete_", "delete"}}),
        MakeRouteSchema<svc::MomentCommentListRequestDto, svc::MomentCommentListResponseDto>(
            "POST",
            "/api/moments/comment/list",
            "moments.comment.list",
            "MomentCommentListRequestDto",
            "MomentCommentListResponseDto"),
        MakeRouteSchema<svc::MomentCommentLikeRequestDto, svc::MomentCommentLikeResponseDto>(
            "POST",
            "/api/moments/comment/like",
            "moments.comment.like",
            "MomentCommentLikeRequestDto",
            "MomentCommentLikeResponseDto"),
    };
}

} // namespace memochat::gate::modules::moments

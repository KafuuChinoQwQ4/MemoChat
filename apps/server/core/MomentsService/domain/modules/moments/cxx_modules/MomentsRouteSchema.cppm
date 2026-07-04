export module memochat.moments.route_schema_algorithms;

export namespace memochat::moments::route_schema::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* PublishPath()
{
    return "/api/moments/publish";
}

const char* PublishRouteName()
{
    return "moments.publish";
}

const char* PublishRequestTypeName()
{
    return "MomentPublishRequestDto";
}

const char* MomentIdResponseTypeName()
{
    return "MomentIdResponseDto";
}

const char* ListPath()
{
    return "/api/moments/list";
}

const char* ListRouteName()
{
    return "moments.list";
}

const char* ListRequestTypeName()
{
    return "MomentListRequestDto";
}

const char* ListResponseTypeName()
{
    return "MomentListResponseDto";
}

const char* DetailPath()
{
    return "/api/moments/detail";
}

const char* DetailRouteName()
{
    return "moments.detail";
}

const char* MomentIdRequestTypeName()
{
    return "MomentIdRequestDto";
}

const char* DetailResponseTypeName()
{
    return "MomentDetailResponseDto";
}

const char* DeletePath()
{
    return "/api/moments/delete";
}

const char* DeleteRouteName()
{
    return "moments.delete";
}

const char* LikePath()
{
    return "/api/moments/like";
}

const char* LikeRouteName()
{
    return "moments.like";
}

const char* LikeRequestTypeName()
{
    return "MomentLikeRequestDto";
}

const char* LikeResponseTypeName()
{
    return "MomentLikeResponseDto";
}

const char* CommentPath()
{
    return "/api/moments/comment";
}

const char* CommentRouteName()
{
    return "moments.comment";
}

const char* CommentRequestTypeName()
{
    return "MomentCommentRequestDto";
}

const char* CommentMutationResponseTypeName()
{
    return "MomentCommentMutationResponseDto";
}

const char* CommentListPath()
{
    return "/api/moments/comment/list";
}

const char* CommentListRouteName()
{
    return "moments.comment.list";
}

const char* CommentListRequestTypeName()
{
    return "MomentCommentListRequestDto";
}

const char* CommentListResponseTypeName()
{
    return "MomentCommentListResponseDto";
}

const char* CommentLikePath()
{
    return "/api/moments/comment/like";
}

const char* CommentLikeRouteName()
{
    return "moments.comment.like";
}

const char* CommentLikeRequestTypeName()
{
    return "MomentCommentLikeRequestDto";
}

const char* CommentLikeResponseTypeName()
{
    return "MomentCommentLikeResponseDto";
}
} // namespace memochat::moments::route_schema::modules

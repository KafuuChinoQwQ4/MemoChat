export module memochat.moments.service_algorithms;

export namespace memochat::moments::service::modules
{
const char* DefaultRelationQueryEndpoint()
{
    return "127.0.0.1:50090";
}

bool ShouldUseDefaultRelationEndpoint(bool endpoint_empty)
{
    return endpoint_empty;
}

int PublicVisibility()
{
    return 0;
}

int FriendsOnlyVisibility()
{
    return 1;
}

bool HasValidViewerAndAuthor(int viewer_uid, int author_uid)
{
    return viewer_uid > 0 && author_uid > 0;
}

bool CanViewWithoutRelationCheck(int viewer_uid, int author_uid, int visibility)
{
    return viewer_uid == author_uid || visibility == PublicVisibility();
}

bool ShouldRejectNonFriendsVisibility(int visibility)
{
    return visibility != FriendsOnlyVisibility();
}

bool IsFriendsOnlyForeignMoment(int visibility, int viewer_uid, int author_uid)
{
    return visibility == FriendsOnlyVisibility() && viewer_uid != author_uid;
}

int SuccessHttpStatus()
{
    return 200;
}

const char* JsonContentType()
{
    return "application/json";
}

bool HasValidUid(int uid)
{
    return uid > 0;
}

bool HasValidMomentId(long long moment_id)
{
    return moment_id > 0;
}

bool HasValidCommentId(long long comment_id)
{
    return comment_id > 0;
}

bool HasValidNewComment(long long moment_id, bool content_empty)
{
    return HasValidMomentId(moment_id) && !content_empty;
}

int CommentLikeFetchLimit()
{
    return 100;
}

int DetailLikeFetchLimit()
{
    return 100;
}

int DetailCommentFetchLimit()
{
    return 100;
}
} // namespace memochat::moments::service::modules

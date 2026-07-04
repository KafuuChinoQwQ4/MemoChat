import memochat.moments.service_algorithms;

namespace memochat::tests::moments::service
{
const char* DefaultRelationQueryEndpoint()
{
    return memochat::moments::service::modules::DefaultRelationQueryEndpoint();
}

bool ShouldUseDefaultRelationEndpoint(bool endpoint_empty)
{
    return memochat::moments::service::modules::ShouldUseDefaultRelationEndpoint(endpoint_empty);
}

int PublicVisibility()
{
    return memochat::moments::service::modules::PublicVisibility();
}

int FriendsOnlyVisibility()
{
    return memochat::moments::service::modules::FriendsOnlyVisibility();
}

bool HasValidViewerAndAuthor(int viewer_uid, int author_uid)
{
    return memochat::moments::service::modules::HasValidViewerAndAuthor(viewer_uid, author_uid);
}

bool CanViewWithoutRelationCheck(int viewer_uid, int author_uid, int visibility)
{
    return memochat::moments::service::modules::CanViewWithoutRelationCheck(viewer_uid, author_uid, visibility);
}

bool ShouldRejectNonFriendsVisibility(int visibility)
{
    return memochat::moments::service::modules::ShouldRejectNonFriendsVisibility(visibility);
}

bool IsFriendsOnlyForeignMoment(int visibility, int viewer_uid, int author_uid)
{
    return memochat::moments::service::modules::IsFriendsOnlyForeignMoment(visibility, viewer_uid, author_uid);
}

int SuccessHttpStatus()
{
    return memochat::moments::service::modules::SuccessHttpStatus();
}

const char* JsonContentType()
{
    return memochat::moments::service::modules::JsonContentType();
}

const char* UidField()
{
    return memochat::moments::service::modules::UidField();
}

const char* LoginTicketField()
{
    return memochat::moments::service::modules::LoginTicketField();
}

bool HasRequiredAuthFields(bool has_uid, bool has_login_ticket)
{
    return memochat::moments::service::modules::HasRequiredAuthFields(has_uid, has_login_ticket);
}

bool HasValidUid(int uid)
{
    return memochat::moments::service::modules::HasValidUid(uid);
}

bool HasValidMomentId(long long moment_id)
{
    return memochat::moments::service::modules::HasValidMomentId(moment_id);
}

bool HasValidCommentId(long long comment_id)
{
    return memochat::moments::service::modules::HasValidCommentId(comment_id);
}

bool HasValidNewComment(long long moment_id, bool content_empty)
{
    return memochat::moments::service::modules::HasValidNewComment(moment_id, content_empty);
}

int CommentLikeFetchLimit()
{
    return memochat::moments::service::modules::CommentLikeFetchLimit();
}

int DetailLikeFetchLimit()
{
    return memochat::moments::service::modules::DetailLikeFetchLimit();
}

int DetailCommentFetchLimit()
{
    return memochat::moments::service::modules::DetailCommentFetchLimit();
}
} // namespace memochat::tests::moments::service

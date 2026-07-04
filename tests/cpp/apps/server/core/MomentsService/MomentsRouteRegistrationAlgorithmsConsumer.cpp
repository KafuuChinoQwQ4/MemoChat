import memochat.moments.route_registration_algorithms;

namespace memochat::tests::moments::route_registration
{
const char* PostMethod()
{
    return memochat::moments::route_registration::modules::PostMethod();
}

const char* PublishPath()
{
    return memochat::moments::route_registration::modules::PublishPath();
}

const char* ListPath()
{
    return memochat::moments::route_registration::modules::ListPath();
}

const char* DetailPath()
{
    return memochat::moments::route_registration::modules::DetailPath();
}

const char* DeletePath()
{
    return memochat::moments::route_registration::modules::DeletePath();
}

const char* LikePath()
{
    return memochat::moments::route_registration::modules::LikePath();
}

const char* CommentPath()
{
    return memochat::moments::route_registration::modules::CommentPath();
}

const char* CommentListPath()
{
    return memochat::moments::route_registration::modules::CommentListPath();
}

const char* CommentLikePath()
{
    return memochat::moments::route_registration::modules::CommentLikePath();
}
} // namespace memochat::tests::moments::route_registration

export module memochat.moments.output_algorithms;

export namespace memochat::gate::services::moments::output::modules
{
bool ShouldKeepLikeName(bool user_nick_empty)
{
    return !user_nick_empty;
}

bool ShouldProjectContent(bool content_present)
{
    return content_present;
}

bool ShouldProjectLikes(bool likes_present)
{
    return likes_present;
}

bool ShouldProjectComments(bool comments_present)
{
    return comments_present;
}
} // namespace memochat::gate::services::moments::output::modules

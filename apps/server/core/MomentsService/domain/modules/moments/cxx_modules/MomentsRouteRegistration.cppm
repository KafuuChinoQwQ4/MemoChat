export module memochat.moments.route_registration_algorithms;

export namespace memochat::moments::route_registration::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* PublishPath()
{
    return "/api/moments/publish";
}

const char* ListPath()
{
    return "/api/moments/list";
}

const char* DetailPath()
{
    return "/api/moments/detail";
}

const char* DeletePath()
{
    return "/api/moments/delete";
}

const char* LikePath()
{
    return "/api/moments/like";
}

const char* CommentPath()
{
    return "/api/moments/comment";
}

const char* CommentListPath()
{
    return "/api/moments/comment/list";
}

const char* CommentLikePath()
{
    return "/api/moments/comment/like";
}
} // namespace memochat::moments::route_registration::modules

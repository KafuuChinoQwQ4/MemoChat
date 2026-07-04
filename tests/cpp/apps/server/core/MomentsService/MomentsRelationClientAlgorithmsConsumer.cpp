import memochat.moments.relation_client_algorithms;

namespace memochat::tests::moments::relation_client
{
const char* ViewerUidField()
{
    return memochat::moments::relation_client::modules::ViewerUidField();
}

const char* AuthorUidsField()
{
    return memochat::moments::relation_client::modules::AuthorUidsField();
}

const char* FriendUidsField()
{
    return memochat::moments::relation_client::modules::FriendUidsField();
}

const char* CompactIndentation()
{
    return memochat::moments::relation_client::modules::CompactIndentation();
}

int RpcDeadlineSeconds()
{
    return memochat::moments::relation_client::modules::RpcDeadlineSeconds();
}

bool ShouldSkipFilter(bool endpoint_empty, int viewer_uid, bool author_uids_empty)
{
    return memochat::moments::relation_client::modules::ShouldSkipFilter(endpoint_empty, viewer_uid, author_uids_empty);
}

bool IsEnabled(bool endpoint_empty)
{
    return memochat::moments::relation_client::modules::IsEnabled(endpoint_empty);
}
} // namespace memochat::tests::moments::relation_client

export module memochat.moments.relation_client_algorithms;

export namespace memochat::moments::relation_client::modules
{
// Request/response JSON field names for the relation-query gRPC payload.
const char* ViewerUidField()
{
    return "viewer_uid";
}

const char* AuthorUidsField()
{
    return "author_uids";
}

const char* FriendUidsField()
{
    return "friend_uids";
}

// Compact JSON writer indentation (no pretty-printing).
const char* CompactIndentation()
{
    return "";
}

// Deadline (seconds) applied to the FilterFriendUids RPC ClientContext.
int RpcDeadlineSeconds()
{
    return 2;
}

// Fail-closed guard: skip the RPC entirely when the endpoint is unset, the
// viewer uid is non-positive, or there are no author uids to filter.
bool ShouldSkipFilter(bool endpoint_empty, int viewer_uid, bool author_uids_empty)
{
    return endpoint_empty || viewer_uid <= 0 || author_uids_empty;
}

// Whether the client is enabled (has a configured endpoint).
bool IsEnabled(bool endpoint_empty)
{
    return !endpoint_empty;
}
} // namespace memochat::moments::relation_client::modules

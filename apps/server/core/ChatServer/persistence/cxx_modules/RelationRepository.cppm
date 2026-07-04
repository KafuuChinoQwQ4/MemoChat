export module memochat.chat.relation_repository_algorithms;

export namespace memochat::chat::persistence::relation_repository::modules
{
constexpr bool IsPositiveUid(int uid)
{
    return uid > 0;
}

constexpr bool IsPositiveGroupId(long long group_id)
{
    return group_id > 0;
}

constexpr bool ShouldQueryPrivateFriendship(int self_id, int friend_id)
{
    return IsPositiveUid(self_id) && IsPositiveUid(friend_id) && self_id != friend_id;
}

constexpr bool ShouldFilterFriendUids(int viewer_uid, bool author_list_empty)
{
    return IsPositiveUid(viewer_uid) && !author_list_empty;
}

constexpr bool ShouldQueryGroupMembership(long long group_id, int uid)
{
    return IsPositiveGroupId(group_id) && IsPositiveUid(uid);
}
} // namespace memochat::chat::persistence::relation_repository::modules

import memochat.chat.relation_repository_algorithms;

bool MemoChatTestRelationRepositoryShouldQueryPrivateFriendship(int self_id, int friend_id)
{
    return memochat::chat::persistence::relation_repository::modules::ShouldQueryPrivateFriendship(self_id, friend_id);
}

bool MemoChatTestRelationRepositoryShouldFilterFriendUids(int viewer_uid, bool author_list_empty)
{
    return memochat::chat::persistence::relation_repository::modules::ShouldFilterFriendUids(viewer_uid,
                                                                                             author_list_empty);
}

bool MemoChatTestRelationRepositoryShouldQueryGroupMembership(long long group_id, int uid)
{
    return memochat::chat::persistence::relation_repository::modules::ShouldQueryGroupMembership(group_id, uid);
}

export module memochat.chat.relation_service_algorithms;

export namespace memochat::chat::relation_service::modules
{
const char* PrivateDialogType()
{
    return "private";
}

const char* GroupDialogType()
{
    return "group";
}

const char* RelationNotifyTaskName()
{
    return "relation_notify";
}

const char* FriendApplyCreatedEvent()
{
    return "friend_apply_created";
}

const char* FriendApplyApprovedEvent()
{
    return "friend_apply_approved";
}

const char* FriendDeletedEvent()
{
    return "friend_deleted";
}

int InternalReadResponseMessageId()
{
    return 0;
}

bool ShouldRejectSearchUserRequest(bool has_user_id, bool user_id_empty)
{
    return !has_user_id || user_id_empty;
}

bool ShouldRejectSearchUserResult(bool found_user, int uid)
{
    return !found_user || uid <= 0;
}

bool ShouldFilterFriendUids(int viewer_uid, bool has_author_uids)
{
    return viewer_uid > 0 && has_author_uids;
}

bool ShouldRejectPositiveUid(int uid)
{
    return uid <= 0;
}

int ResolveAuthenticatedUid(int payload_uid, int session_uid)
{
    return session_uid > 0 ? session_uid : payload_uid;
}

bool ShouldRejectDeleteFriend(int uid, int friend_uid)
{
    return uid <= 0 || friend_uid <= 0 || uid == friend_uid;
}

bool ShouldRejectDialogType(bool matches_private, bool matches_group)
{
    return !matches_private && !matches_group;
}

bool ShouldRejectPrivateDialogTarget(int peer_uid, bool is_private_friend)
{
    return peer_uid <= 0 || !is_private_friend;
}

bool ShouldRejectGroupDialogTarget(long long group_id, bool is_group_member)
{
    return group_id <= 0 || !is_group_member;
}

int NormalizeMuteState(int mute_state)
{
    return mute_state > 0 ? 1 : 0;
}
} // namespace memochat::chat::relation_service::modules

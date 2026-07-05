import memochat.chat.relation_service_algorithms;

namespace memochat::tests::chat::relation_service
{
const char* PrivateDialogType()
{
    return memochat::chat::relation_service::modules::PrivateDialogType();
}

const char* GroupDialogType()
{
    return memochat::chat::relation_service::modules::GroupDialogType();
}

const char* RelationNotifyTaskName()
{
    return memochat::chat::relation_service::modules::RelationNotifyTaskName();
}

const char* FriendApplyCreatedEvent()
{
    return memochat::chat::relation_service::modules::FriendApplyCreatedEvent();
}

const char* FriendApplyApprovedEvent()
{
    return memochat::chat::relation_service::modules::FriendApplyApprovedEvent();
}

const char* FriendDeletedEvent()
{
    return memochat::chat::relation_service::modules::FriendDeletedEvent();
}

int InternalReadResponseMessageId()
{
    return memochat::chat::relation_service::modules::InternalReadResponseMessageId();
}

bool ShouldRejectSearchUserRequest(bool has_user_id, bool user_id_empty)
{
    return memochat::chat::relation_service::modules::ShouldRejectSearchUserRequest(has_user_id, user_id_empty);
}

bool ShouldRejectSearchUserResult(bool found_user, int uid)
{
    return memochat::chat::relation_service::modules::ShouldRejectSearchUserResult(found_user, uid);
}

bool ShouldFilterFriendUids(int viewer_uid, bool has_author_uids)
{
    return memochat::chat::relation_service::modules::ShouldFilterFriendUids(viewer_uid, has_author_uids);
}

bool ShouldRejectPositiveUid(int uid)
{
    return memochat::chat::relation_service::modules::ShouldRejectPositiveUid(uid);
}

int ResolveAuthenticatedUid(int payload_uid, int session_uid)
{
    return memochat::chat::relation_service::modules::ResolveAuthenticatedUid(payload_uid, session_uid);
}

bool ShouldRejectDeleteFriend(int uid, int friend_uid)
{
    return memochat::chat::relation_service::modules::ShouldRejectDeleteFriend(uid, friend_uid);
}

bool ShouldRejectDialogType(bool matches_private, bool matches_group)
{
    return memochat::chat::relation_service::modules::ShouldRejectDialogType(matches_private, matches_group);
}

bool ShouldRejectPrivateDialogTarget(int peer_uid, bool is_private_friend)
{
    return memochat::chat::relation_service::modules::ShouldRejectPrivateDialogTarget(peer_uid, is_private_friend);
}

bool ShouldRejectGroupDialogTarget(long long group_id, bool is_group_member)
{
    return memochat::chat::relation_service::modules::ShouldRejectGroupDialogTarget(group_id, is_group_member);
}

int NormalizeMuteState(int mute_state)
{
    return memochat::chat::relation_service::modules::NormalizeMuteState(mute_state);
}
} // namespace memochat::tests::chat::relation_service

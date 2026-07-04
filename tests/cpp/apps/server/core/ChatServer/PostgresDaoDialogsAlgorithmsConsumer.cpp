import memochat.chat.postgres_dao_dialogs_algorithms;

namespace postgres_dao_dialogs_modules = memochat::chat::persistence::postgres_dao_dialogs::modules;

bool MemoChatTestPostgresDaoDialogsCanLoadMeta(int owner_uid)
{
    return postgres_dao_dialogs_modules::CanLoadDialogMeta(owner_uid);
}

bool MemoChatTestPostgresDaoDialogsCanReadPrivateRuntime(int owner_uid, int peer_uid)
{
    return postgres_dao_dialogs_modules::CanReadPrivateDialogRuntime(owner_uid, peer_uid);
}

bool MemoChatTestPostgresDaoDialogsCanReadGroupRuntime(int owner_uid, long long group_id)
{
    return postgres_dao_dialogs_modules::CanReadGroupDialogRuntime(owner_uid, group_id);
}

bool MemoChatTestPostgresDaoDialogsKeepUid(int uid)
{
    return postgres_dao_dialogs_modules::ShouldKeepPositiveUid(uid);
}

bool MemoChatTestPostgresDaoDialogsKeepGroup(long long group_id)
{
    return postgres_dao_dialogs_modules::ShouldKeepPositiveGroupId(group_id);
}

int MemoChatTestPostgresDaoDialogsConversationMin(int left_uid, int right_uid)
{
    return postgres_dao_dialogs_modules::ConversationUidMin(left_uid, right_uid);
}

int MemoChatTestPostgresDaoDialogsConversationMax(int left_uid, int right_uid)
{
    return postgres_dao_dialogs_modules::ConversationUidMax(left_uid, right_uid);
}

int MemoChatTestPostgresDaoDialogsNormalizeUnread(int unread_count)
{
    return postgres_dao_dialogs_modules::NormalizeUnreadCount(unread_count);
}

bool MemoChatTestPostgresDaoDialogsCanUpsertGroupRead(int uid, long long group_id)
{
    return postgres_dao_dialogs_modules::CanUpsertGroupReadState(uid, group_id);
}

bool MemoChatTestPostgresDaoDialogsFallbackTimestamp(long long timestamp_ms)
{
    return postgres_dao_dialogs_modules::ShouldUseFallbackTimestamp(timestamp_ms);
}

bool MemoChatTestPostgresDaoDialogsKnownType(bool is_private, bool is_group)
{
    return postgres_dao_dialogs_modules::HasKnownDialogType(is_private, is_group);
}

int MemoChatTestPostgresDaoDialogsNormalizePeer(bool is_private, int peer_uid)
{
    return postgres_dao_dialogs_modules::NormalizeDialogPeerUid(is_private, peer_uid);
}

long long MemoChatTestPostgresDaoDialogsNormalizeGroup(bool is_group, long long group_id)
{
    return postgres_dao_dialogs_modules::NormalizeDialogGroupId(is_group, group_id);
}

bool MemoChatTestPostgresDaoDialogsCanUpsertDialog(int owner_uid,
                                                   bool is_private,
                                                   bool is_group,
                                                   int normalized_peer_uid,
                                                   long long normalized_group_id)
{
    return postgres_dao_dialogs_modules::CanUpsertDialogMeta(owner_uid,
                                                             is_private,
                                                             is_group,
                                                             normalized_peer_uid,
                                                             normalized_group_id);
}

int MemoChatTestPostgresDaoDialogsMaxDraftLength()
{
    return postgres_dao_dialogs_modules::MaxDraftLength();
}

int MemoChatTestPostgresDaoDialogsNormalizePinned(int pinned_rank)
{
    return postgres_dao_dialogs_modules::NormalizePinnedRank(pinned_rank);
}

int MemoChatTestPostgresDaoDialogsNormalizeMute(int mute_state)
{
    return postgres_dao_dialogs_modules::NormalizeMuteState(mute_state);
}

int MemoChatTestPostgresDaoDialogsClampApplyLimit(int limit)
{
    return postgres_dao_dialogs_modules::ClampGroupApplyLimit(limit);
}

bool MemoChatTestPostgresDaoDialogsGroupCodeHeader(int length, char prefix, char first_digit)
{
    return postgres_dao_dialogs_modules::HasGroupCodeHeader(length, prefix, first_digit);
}

bool MemoChatTestPostgresDaoDialogsGroupCodeTail(char c)
{
    return postgres_dao_dialogs_modules::IsGroupCodeTailChar(c);
}

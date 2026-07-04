export module memochat.chat.postgres_dao_dialogs_algorithms;

export namespace memochat::chat::persistence::postgres_dao_dialogs::modules
{
int MinInt(int left, int right)
{
    return left < right ? left : right;
}

int MaxInt(int left, int right)
{
    return left > right ? left : right;
}

bool HasPositiveUid(int uid)
{
    return uid > 0;
}

bool HasPositiveGroupId(long long group_id)
{
    return group_id > 0;
}

bool CanLoadDialogMeta(int owner_uid)
{
    return HasPositiveUid(owner_uid);
}

bool CanReadPrivateDialogRuntime(int owner_uid, int peer_uid)
{
    return HasPositiveUid(owner_uid) && HasPositiveUid(peer_uid);
}

bool CanReadGroupDialogRuntime(int owner_uid, long long group_id)
{
    return HasPositiveUid(owner_uid) && HasPositiveGroupId(group_id);
}

bool ShouldKeepPositiveUid(int uid)
{
    return HasPositiveUid(uid);
}

bool ShouldKeepPositiveGroupId(long long group_id)
{
    return HasPositiveGroupId(group_id);
}

int ConversationUidMin(int left_uid, int right_uid)
{
    return MinInt(left_uid, right_uid);
}

int ConversationUidMax(int left_uid, int right_uid)
{
    return MaxInt(left_uid, right_uid);
}

int NormalizeUnreadCount(int unread_count)
{
    return MaxInt(0, unread_count);
}

bool CanUpsertGroupReadState(int uid, long long group_id)
{
    return HasPositiveUid(uid) && HasPositiveGroupId(group_id);
}

bool ShouldUseFallbackTimestamp(long long timestamp_ms)
{
    return timestamp_ms <= 0;
}

bool HasKnownDialogType(bool is_private, bool is_group)
{
    return is_private != is_group;
}

int NormalizeDialogPeerUid(bool is_private, int peer_uid)
{
    return is_private ? peer_uid : 0;
}

long long NormalizeDialogGroupId(bool is_group, long long group_id)
{
    return is_group ? group_id : 0;
}

bool HasValidDialogTarget(bool is_private, bool is_group, int normalized_peer_uid, long long normalized_group_id)
{
    return (is_private && HasPositiveUid(normalized_peer_uid)) ||
           (is_group && HasPositiveGroupId(normalized_group_id));
}

bool CanUpsertDialogMeta(int owner_uid,
                         bool is_private,
                         bool is_group,
                         int normalized_peer_uid,
                         long long normalized_group_id)
{
    return HasPositiveUid(owner_uid) && HasKnownDialogType(is_private, is_group) &&
           HasValidDialogTarget(is_private, is_group, normalized_peer_uid, normalized_group_id);
}

int MaxDraftLength()
{
    return 2000;
}

int NormalizePinnedRank(int pinned_rank)
{
    return MaxInt(0, pinned_rank);
}

int NormalizeMuteState(int mute_state)
{
    return mute_state > 0 ? 1 : 0;
}

int GroupApplyLimitMax()
{
    return 100;
}

int ClampGroupApplyLimit(int limit)
{
    return MaxInt(1, MinInt(limit, GroupApplyLimitMax()));
}

int GroupCodeLength()
{
    return 10;
}

char GroupCodePrefix()
{
    return 'g';
}

bool IsAsciiDigit(char c)
{
    return c >= '0' && c <= '9';
}

bool IsNonZeroAsciiDigit(char c)
{
    return c >= '1' && c <= '9';
}

bool HasGroupCodeHeader(int length, char prefix, char first_digit)
{
    return length == GroupCodeLength() && prefix == GroupCodePrefix() && IsNonZeroAsciiDigit(first_digit);
}

bool IsGroupCodeTailChar(char c)
{
    return IsAsciiDigit(c);
}
} // namespace memochat::chat::persistence::postgres_dao_dialogs::modules

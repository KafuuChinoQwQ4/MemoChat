export module memochat.chat.postgres_dao_group_messages_algorithms;

export namespace memochat::chat::persistence::postgres_dao_group_messages::modules
{
long long DeleteMessagesPermissionBit()
{
    return 1LL << 1;
}

long long MessageRevokeWindowMs()
{
    return 5LL * 60LL * 1000LL;
}

int MinInt(int left, int right)
{
    return left < right ? left : right;
}

int MaxInt(int left, int right)
{
    return left > right ? left : right;
}

bool HasPositiveGroupId(long long group_id)
{
    return group_id > 0;
}

bool HasPositiveUid(int uid)
{
    return uid > 0;
}

bool CanSaveGroupMessage(bool msg_id_empty, long long group_id, int from_uid)
{
    return !msg_id_empty && HasPositiveGroupId(group_id) && HasPositiveUid(from_uid);
}

bool ShouldLoadNextGroupSeq(long long assigned_group_seq)
{
    return assigned_group_seq <= 0;
}

long long NormalizeNextGroupSeq(long long candidate_group_seq)
{
    return candidate_group_seq > 0 ? candidate_group_seq : 1;
}

bool ShouldWriteGroupMessageExt(bool file_name_empty, bool mime_empty, int size)
{
    return !file_name_empty || !mime_empty || size > 0;
}

bool CanUpdateGroupMessage(long long group_id, int operator_uid, bool msg_id_empty, bool content_empty)
{
    return HasPositiveGroupId(group_id) && HasPositiveUid(operator_uid) && !msg_id_empty && !content_empty;
}

bool ShouldUseFallbackTimestamp(long long timestamp_ms)
{
    return timestamp_ms <= 0;
}

bool HasDeleteMessagesPermission(long long permissions)
{
    return (permissions & DeleteMessagesPermissionBit()) != 0;
}

bool CanOperatorEditGroupMessage(int from_uid, int operator_uid, bool has_delete_permission)
{
    return from_uid == operator_uid || has_delete_permission;
}

bool CanRequestGroupMessageRevoke(long long group_id, int operator_uid, bool msg_id_empty)
{
    return HasPositiveGroupId(group_id) && HasPositiveUid(operator_uid) && !msg_id_empty;
}

bool IsWithinRevokeWindow(long long deleted_at_ms, long long created_at, long long revoke_window_ms)
{
    return created_at > 0 && deleted_at_ms - created_at <= revoke_window_ms;
}

bool CanRevokeGroupMessage(int from_uid,
                           int operator_uid,
                           long long existing_deleted_at_ms,
                           long long created_at,
                           long long deleted_at_ms,
                           long long revoke_window_ms)
{
    return from_uid == operator_uid && existing_deleted_at_ms <= 0 &&
           IsWithinRevokeWindow(deleted_at_ms, created_at, revoke_window_ms);
}

long long RevokeWindowStart(long long deleted_at_ms, long long revoke_window_ms)
{
    return deleted_at_ms - revoke_window_ms;
}

int ClampHistoryLimit(int limit)
{
    return MaxInt(1, MinInt(limit, 50));
}

int SelectHistoryFetchLimit(int final_limit)
{
    return final_limit + 1;
}

bool ShouldApplyGroupSeqCursor(long long before_seq)
{
    return before_seq > 0;
}

bool ShouldApplyTimestampCursor(long long before_seq, long long before_ts)
{
    return before_seq <= 0 && before_ts > 0;
}

bool HasOverflowPage(int loaded_count, int final_limit)
{
    return loaded_count > final_limit;
}

bool CanFindGroupMessage(long long group_id, bool msg_id_empty)
{
    return HasPositiveGroupId(group_id) && !msg_id_empty;
}
} // namespace memochat::chat::persistence::postgres_dao_group_messages::modules

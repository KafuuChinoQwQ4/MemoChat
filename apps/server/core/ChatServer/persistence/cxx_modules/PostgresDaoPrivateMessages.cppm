export module memochat.chat.postgres_dao_private_messages_algorithms;

export namespace memochat::chat::persistence::postgres_dao_private_messages::modules
{
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

int ConversationUidMin(int uid, int peer_uid)
{
    return MinInt(uid, peer_uid);
}

int ConversationUidMax(int uid, int peer_uid)
{
    return MaxInt(uid, peer_uid);
}

bool HasPositiveUid(int uid)
{
    return uid > 0;
}

bool CanReadUndeliveredPrivateMessages(int to_uid, int limit)
{
    return HasPositiveUid(to_uid) && limit > 0;
}

bool CanReadPrivateHistory(int uid, int peer_uid, int limit)
{
    return HasPositiveUid(uid) && HasPositiveUid(peer_uid) && limit > 0;
}

int SelectHistoryFetchLimit(int requested_limit)
{
    return requested_limit + 1;
}

bool ShouldApplyHistoryTieBreaker(long long before_ts, bool before_msg_id_empty)
{
    return before_ts > 0 && !before_msg_id_empty;
}

bool ShouldApplyTimestampCursor(long long before_ts)
{
    return before_ts > 0;
}

bool HasOverflowPage(int loaded_count, int requested_limit)
{
    return loaded_count > requested_limit;
}

bool CanFindPrivateMessage(bool msg_id_empty)
{
    return !msg_id_empty;
}

bool CanUpdatePrivateMessage(int uid, int peer_uid, bool msg_id_empty, bool content_empty)
{
    return HasPositiveUid(uid) && HasPositiveUid(peer_uid) && !msg_id_empty && !content_empty;
}

bool ShouldUseFallbackTimestamp(long long timestamp_ms)
{
    return timestamp_ms <= 0;
}

bool IsPrivateMessageOwner(int message_conv_uid_min,
                           int message_conv_uid_max,
                           int message_from_uid,
                           int uid,
                           int peer_uid)
{
    return message_conv_uid_min == ConversationUidMin(uid, peer_uid) &&
           message_conv_uid_max == ConversationUidMax(uid, peer_uid) && message_from_uid == uid;
}

bool CanRequestPrivateMessageRevoke(int uid, int peer_uid, bool msg_id_empty)
{
    return HasPositiveUid(uid) && HasPositiveUid(peer_uid) && !msg_id_empty;
}

bool IsWithinRevokeWindow(long long deleted_at_ms, long long created_at_ms, long long revoke_window_ms)
{
    return created_at_ms > 0 && deleted_at_ms - created_at_ms <= revoke_window_ms;
}

bool CanRevokePrivateMessage(bool owner_matches,
                             long long existing_deleted_at_ms,
                             long long created_at_ms,
                             long long deleted_at_ms,
                             long long revoke_window_ms)
{
    return owner_matches && existing_deleted_at_ms <= 0 &&
           IsWithinRevokeWindow(deleted_at_ms, created_at_ms, revoke_window_ms);
}

long long RevokeWindowStart(long long deleted_at_ms, long long revoke_window_ms)
{
    return deleted_at_ms - revoke_window_ms;
}

bool CanUpsertPrivateReadState(int uid, int peer_uid)
{
    return HasPositiveUid(uid) && HasPositiveUid(peer_uid);
}
} // namespace memochat::chat::persistence::postgres_dao_private_messages::modules

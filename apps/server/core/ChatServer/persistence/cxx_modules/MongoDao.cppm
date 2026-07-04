export module memochat.chat.mongo_dao_algorithms;

export namespace memochat::chat::persistence::mongo_dao::modules
{
char ToLowerAscii(char ch)
{
    return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
}

bool EqualsIgnoreCaseAscii(const char* text, const char* expected)
{
    if (text == nullptr || expected == nullptr)
    {
        return false;
    }
    while (*text != '\0' && *expected != '\0')
    {
        if (ToLowerAscii(*text) != *expected)
        {
            return false;
        }
        ++text;
        ++expected;
    }
    return *text == '\0' && *expected == '\0';
}

bool ParseBoolText(const char* text)
{
    return EqualsIgnoreCaseAscii(text, "1") || EqualsIgnoreCaseAscii(text, "true") ||
           EqualsIgnoreCaseAscii(text, "yes") || EqualsIgnoreCaseAscii(text, "on");
}

bool IsEnabled(bool enabled, bool init_ok, bool has_pool)
{
    return enabled && init_ok && has_pool;
}

bool ShouldInitializeMongo(bool enabled)
{
    return enabled;
}

bool HasRequiredConfig(bool uri_empty, bool database_empty)
{
    return !uri_empty && !database_empty;
}

bool ShouldUseDefaultCollectionName(bool collection_name_empty)
{
    return collection_name_empty;
}

bool CanEnsureIndexes(bool has_pool)
{
    return has_pool;
}

int MinInt(int left, int right)
{
    return left < right ? left : right;
}

int MaxInt(int left, int right)
{
    return left > right ? left : right;
}

int ClampHistoryLimit(int limit)
{
    return MaxInt(1, MinInt(limit, 50));
}

bool HasMorePage(int loaded_count, int final_limit)
{
    return loaded_count > final_limit;
}

bool CanSavePrivateMessage(bool enabled, bool msg_id_empty)
{
    return enabled && !msg_id_empty;
}

bool CanReadPrivateHistory(bool enabled, int uid, int peer_uid, int limit)
{
    return enabled && uid > 0 && peer_uid > 0 && limit > 0;
}

bool ShouldApplyPrivateTieBreaker(long long before_ts, bool before_msg_id_empty)
{
    return before_ts > 0 && !before_msg_id_empty;
}

bool ShouldApplyTimestampFilter(long long value)
{
    return value > 0;
}

bool CanFindPrivateMessage(bool enabled, bool msg_id_empty)
{
    return enabled && !msg_id_empty;
}

bool CanUpdatePrivateMessage(bool enabled, int uid, int peer_uid, bool msg_id_empty, bool content_empty)
{
    return enabled && uid > 0 && peer_uid > 0 && !msg_id_empty && !content_empty;
}

bool CanRequestPrivateMessageRevoke(bool enabled, int uid, int peer_uid, bool msg_id_empty)
{
    return enabled && uid > 0 && peer_uid > 0 && !msg_id_empty;
}

long long SelectOperationTimestamp(long long requested_timestamp, long long fallback_now_ms)
{
    return requested_timestamp > 0 ? requested_timestamp : fallback_now_ms;
}

bool IsPrivateMessageOwner(int message_conv_uid_min,
                           int message_conv_uid_max,
                           int message_from_uid,
                           int uid,
                           int peer_uid)
{
    return message_conv_uid_min == MinInt(uid, peer_uid) && message_conv_uid_max == MaxInt(uid, peer_uid) &&
           message_from_uid == uid;
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

bool CanSaveGroupMessage(bool enabled, bool msg_id_empty, long long group_id)
{
    return enabled && !msg_id_empty && group_id > 0;
}

bool CanReadGroupHistory(bool enabled, long long group_id, int limit)
{
    return enabled && group_id > 0 && limit > 0;
}

bool ShouldApplyGroupSeqFilter(long long before_seq)
{
    return before_seq > 0;
}

bool ShouldApplyGroupTimestampFilter(long long before_seq, long long before_ts)
{
    return before_seq <= 0 && before_ts > 0;
}

bool CanFindGroupMessage(bool enabled, long long group_id, bool msg_id_empty)
{
    return enabled && group_id > 0 && !msg_id_empty;
}

bool CanUpdateGroupMessage(bool enabled, long long group_id, bool msg_id_empty, bool content_empty)
{
    return enabled && group_id > 0 && !msg_id_empty && !content_empty;
}

bool CanRequestGroupMessageRevoke(bool enabled, long long group_id, int operator_uid, bool msg_id_empty)
{
    return enabled && group_id > 0 && operator_uid > 0 && !msg_id_empty;
}

bool CanApplyGroupMessageRevoke(int message_from_uid,
                                int operator_uid,
                                long long existing_deleted_at_ms,
                                long long created_at_ms,
                                long long deleted_at_ms,
                                long long revoke_window_ms)
{
    return message_from_uid == operator_uid && existing_deleted_at_ms <= 0 &&
           IsWithinRevokeWindow(deleted_at_ms, created_at_ms, revoke_window_ms);
}
} // namespace memochat::chat::persistence::mongo_dao::modules

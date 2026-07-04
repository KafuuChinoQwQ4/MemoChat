import memochat.chat.postgres_dao_group_messages_algorithms;

namespace postgres_dao_group_messages_modules = memochat::chat::persistence::postgres_dao_group_messages::modules;

long long MemoChatTestPostgresDaoGroupMessagesPermissionBit()
{
    return postgres_dao_group_messages_modules::DeleteMessagesPermissionBit();
}

long long MemoChatTestPostgresDaoGroupMessagesRevokeWindow()
{
    return postgres_dao_group_messages_modules::MessageRevokeWindowMs();
}

bool MemoChatTestPostgresDaoGroupMessagesCanSave(bool msg_id_empty, long long group_id, int from_uid)
{
    return postgres_dao_group_messages_modules::CanSaveGroupMessage(msg_id_empty, group_id, from_uid);
}

bool MemoChatTestPostgresDaoGroupMessagesShouldLoadSeq(long long assigned_group_seq)
{
    return postgres_dao_group_messages_modules::ShouldLoadNextGroupSeq(assigned_group_seq);
}

long long MemoChatTestPostgresDaoGroupMessagesNormalizeSeq(long long candidate_group_seq)
{
    return postgres_dao_group_messages_modules::NormalizeNextGroupSeq(candidate_group_seq);
}

bool MemoChatTestPostgresDaoGroupMessagesWriteExt(bool file_name_empty, bool mime_empty, int size)
{
    return postgres_dao_group_messages_modules::ShouldWriteGroupMessageExt(file_name_empty, mime_empty, size);
}

bool MemoChatTestPostgresDaoGroupMessagesCanUpdate(long long group_id,
                                                   int operator_uid,
                                                   bool msg_id_empty,
                                                   bool content_empty)
{
    return postgres_dao_group_messages_modules::CanUpdateGroupMessage(group_id,
                                                                      operator_uid,
                                                                      msg_id_empty,
                                                                      content_empty);
}

bool MemoChatTestPostgresDaoGroupMessagesFallbackTimestamp(long long timestamp_ms)
{
    return postgres_dao_group_messages_modules::ShouldUseFallbackTimestamp(timestamp_ms);
}

bool MemoChatTestPostgresDaoGroupMessagesHasDeletePermission(long long permissions)
{
    return postgres_dao_group_messages_modules::HasDeleteMessagesPermission(permissions);
}

bool MemoChatTestPostgresDaoGroupMessagesCanEdit(int from_uid, int operator_uid, bool has_delete_permission)
{
    return postgres_dao_group_messages_modules::CanOperatorEditGroupMessage(from_uid,
                                                                            operator_uid,
                                                                            has_delete_permission);
}

bool MemoChatTestPostgresDaoGroupMessagesCanRequestRevoke(long long group_id, int operator_uid, bool msg_id_empty)
{
    return postgres_dao_group_messages_modules::CanRequestGroupMessageRevoke(group_id, operator_uid, msg_id_empty);
}

bool MemoChatTestPostgresDaoGroupMessagesCanRevoke(int from_uid,
                                                   int operator_uid,
                                                   long long existing_deleted_at_ms,
                                                   long long created_at,
                                                   long long deleted_at_ms,
                                                   long long revoke_window_ms)
{
    return postgres_dao_group_messages_modules::CanRevokeGroupMessage(from_uid,
                                                                      operator_uid,
                                                                      existing_deleted_at_ms,
                                                                      created_at,
                                                                      deleted_at_ms,
                                                                      revoke_window_ms);
}

long long MemoChatTestPostgresDaoGroupMessagesRevokeWindowStart(long long deleted_at_ms, long long revoke_window_ms)
{
    return postgres_dao_group_messages_modules::RevokeWindowStart(deleted_at_ms, revoke_window_ms);
}

int MemoChatTestPostgresDaoGroupMessagesClampLimit(int limit)
{
    return postgres_dao_group_messages_modules::ClampHistoryLimit(limit);
}

int MemoChatTestPostgresDaoGroupMessagesFetchLimit(int final_limit)
{
    return postgres_dao_group_messages_modules::SelectHistoryFetchLimit(final_limit);
}

bool MemoChatTestPostgresDaoGroupMessagesSeqCursor(long long before_seq)
{
    return postgres_dao_group_messages_modules::ShouldApplyGroupSeqCursor(before_seq);
}

bool MemoChatTestPostgresDaoGroupMessagesTimestampCursor(long long before_seq, long long before_ts)
{
    return postgres_dao_group_messages_modules::ShouldApplyTimestampCursor(before_seq, before_ts);
}

bool MemoChatTestPostgresDaoGroupMessagesOverflow(int loaded_count, int final_limit)
{
    return postgres_dao_group_messages_modules::HasOverflowPage(loaded_count, final_limit);
}

bool MemoChatTestPostgresDaoGroupMessagesCanFind(long long group_id, bool msg_id_empty)
{
    return postgres_dao_group_messages_modules::CanFindGroupMessage(group_id, msg_id_empty);
}

import memochat.chat.mongo_dao_algorithms;

namespace mongo_dao_modules = memochat::chat::persistence::mongo_dao::modules;

bool MemoChatTestMongoDaoParseBoolText(const char* text)
{
    return mongo_dao_modules::ParseBoolText(text);
}

bool MemoChatTestMongoDaoEnabled(bool enabled, bool init_ok, bool has_pool)
{
    return mongo_dao_modules::IsEnabled(enabled, init_ok, has_pool);
}

bool MemoChatTestMongoDaoShouldInitialize(bool enabled)
{
    return mongo_dao_modules::ShouldInitializeMongo(enabled);
}

bool MemoChatTestMongoDaoHasRequiredConfig(bool uri_empty, bool database_empty)
{
    return mongo_dao_modules::HasRequiredConfig(uri_empty, database_empty);
}

bool MemoChatTestMongoDaoUseDefaultCollection(bool collection_name_empty)
{
    return mongo_dao_modules::ShouldUseDefaultCollectionName(collection_name_empty);
}

int MemoChatTestMongoDaoClampHistoryLimit(int limit)
{
    return mongo_dao_modules::ClampHistoryLimit(limit);
}

bool MemoChatTestMongoDaoHasMorePage(int loaded_count, int final_limit)
{
    return mongo_dao_modules::HasMorePage(loaded_count, final_limit);
}

bool MemoChatTestMongoDaoCanReadPrivateHistory(bool enabled, int uid, int peer_uid, int limit)
{
    return mongo_dao_modules::CanReadPrivateHistory(enabled, uid, peer_uid, limit);
}

bool MemoChatTestMongoDaoPrivateTieBreaker(long long before_ts, bool before_msg_id_empty)
{
    return mongo_dao_modules::ShouldApplyPrivateTieBreaker(before_ts, before_msg_id_empty);
}

bool MemoChatTestMongoDaoTimestampFilter(long long value)
{
    return mongo_dao_modules::ShouldApplyTimestampFilter(value);
}

bool MemoChatTestMongoDaoCanUpdatePrivate(bool enabled, int uid, int peer_uid, bool msg_id_empty, bool content_empty)
{
    return mongo_dao_modules::CanUpdatePrivateMessage(enabled, uid, peer_uid, msg_id_empty, content_empty);
}

bool MemoChatTestMongoDaoCanRequestPrivateRevoke(bool enabled, int uid, int peer_uid, bool msg_id_empty)
{
    return mongo_dao_modules::CanRequestPrivateMessageRevoke(enabled, uid, peer_uid, msg_id_empty);
}

long long MemoChatTestMongoDaoSelectTimestamp(long long requested_timestamp, long long fallback_now_ms)
{
    return mongo_dao_modules::SelectOperationTimestamp(requested_timestamp, fallback_now_ms);
}

bool MemoChatTestMongoDaoPrivateOwner(int conv_min, int conv_max, int from_uid, int uid, int peer_uid)
{
    return mongo_dao_modules::IsPrivateMessageOwner(conv_min, conv_max, from_uid, uid, peer_uid);
}

bool MemoChatTestMongoDaoCanRevokePrivate(bool owner_matches,
                                          long long existing_deleted_at_ms,
                                          long long created_at_ms,
                                          long long deleted_at_ms,
                                          long long revoke_window_ms)
{
    return mongo_dao_modules::CanRevokePrivateMessage(owner_matches,
                                                      existing_deleted_at_ms,
                                                      created_at_ms,
                                                      deleted_at_ms,
                                                      revoke_window_ms);
}

bool MemoChatTestMongoDaoCanSaveGroup(bool enabled, bool msg_id_empty, long long group_id)
{
    return mongo_dao_modules::CanSaveGroupMessage(enabled, msg_id_empty, group_id);
}

bool MemoChatTestMongoDaoCanReadGroupHistory(bool enabled, long long group_id, int limit)
{
    return mongo_dao_modules::CanReadGroupHistory(enabled, group_id, limit);
}

bool MemoChatTestMongoDaoGroupSeqFilter(long long before_seq)
{
    return mongo_dao_modules::ShouldApplyGroupSeqFilter(before_seq);
}

bool MemoChatTestMongoDaoGroupTimestampFilter(long long before_seq, long long before_ts)
{
    return mongo_dao_modules::ShouldApplyGroupTimestampFilter(before_seq, before_ts);
}

bool MemoChatTestMongoDaoCanRequestGroupRevoke(bool enabled, long long group_id, int operator_uid, bool msg_id_empty)
{
    return mongo_dao_modules::CanRequestGroupMessageRevoke(enabled, group_id, operator_uid, msg_id_empty);
}

bool MemoChatTestMongoDaoCanApplyGroupRevoke(int message_from_uid,
                                             int operator_uid,
                                             long long existing_deleted_at_ms,
                                             long long created_at_ms,
                                             long long deleted_at_ms,
                                             long long revoke_window_ms)
{
    return mongo_dao_modules::CanApplyGroupMessageRevoke(message_from_uid,
                                                         operator_uid,
                                                         existing_deleted_at_ms,
                                                         created_at_ms,
                                                         deleted_at_ms,
                                                         revoke_window_ms);
}

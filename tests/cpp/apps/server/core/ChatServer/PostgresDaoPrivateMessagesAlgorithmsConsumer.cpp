import memochat.chat.postgres_dao_private_messages_algorithms;

namespace postgres_dao_private_messages_modules = memochat::chat::persistence::postgres_dao_private_messages::modules;

long long MemoChatTestPostgresDaoPrivateMessagesRevokeWindow()
{
    return postgres_dao_private_messages_modules::MessageRevokeWindowMs();
}

int MemoChatTestPostgresDaoPrivateMessagesConvMin(int uid, int peer_uid)
{
    return postgres_dao_private_messages_modules::ConversationUidMin(uid, peer_uid);
}

int MemoChatTestPostgresDaoPrivateMessagesConvMax(int uid, int peer_uid)
{
    return postgres_dao_private_messages_modules::ConversationUidMax(uid, peer_uid);
}

bool MemoChatTestPostgresDaoPrivateMessagesCanReadUndelivered(int to_uid, int limit)
{
    return postgres_dao_private_messages_modules::CanReadUndeliveredPrivateMessages(to_uid, limit);
}

bool MemoChatTestPostgresDaoPrivateMessagesCanReadHistory(int uid, int peer_uid, int limit)
{
    return postgres_dao_private_messages_modules::CanReadPrivateHistory(uid, peer_uid, limit);
}

int MemoChatTestPostgresDaoPrivateMessagesFetchLimit(int requested_limit)
{
    return postgres_dao_private_messages_modules::SelectHistoryFetchLimit(requested_limit);
}

bool MemoChatTestPostgresDaoPrivateMessagesTieBreaker(long long before_ts, bool before_msg_id_empty)
{
    return postgres_dao_private_messages_modules::ShouldApplyHistoryTieBreaker(before_ts, before_msg_id_empty);
}

bool MemoChatTestPostgresDaoPrivateMessagesTimestampCursor(long long before_ts)
{
    return postgres_dao_private_messages_modules::ShouldApplyTimestampCursor(before_ts);
}

bool MemoChatTestPostgresDaoPrivateMessagesHasOverflow(int loaded_count, int requested_limit)
{
    return postgres_dao_private_messages_modules::HasOverflowPage(loaded_count, requested_limit);
}

bool MemoChatTestPostgresDaoPrivateMessagesCanFind(bool msg_id_empty)
{
    return postgres_dao_private_messages_modules::CanFindPrivateMessage(msg_id_empty);
}

bool MemoChatTestPostgresDaoPrivateMessagesCanUpdate(int uid, int peer_uid, bool msg_id_empty, bool content_empty)
{
    return postgres_dao_private_messages_modules::CanUpdatePrivateMessage(uid, peer_uid, msg_id_empty, content_empty);
}

bool MemoChatTestPostgresDaoPrivateMessagesFallbackTimestamp(long long timestamp_ms)
{
    return postgres_dao_private_messages_modules::ShouldUseFallbackTimestamp(timestamp_ms);
}

bool MemoChatTestPostgresDaoPrivateMessagesOwner(int conv_min, int conv_max, int from_uid, int uid, int peer_uid)
{
    return postgres_dao_private_messages_modules::IsPrivateMessageOwner(conv_min, conv_max, from_uid, uid, peer_uid);
}

bool MemoChatTestPostgresDaoPrivateMessagesCanRequestRevoke(int uid, int peer_uid, bool msg_id_empty)
{
    return postgres_dao_private_messages_modules::CanRequestPrivateMessageRevoke(uid, peer_uid, msg_id_empty);
}

bool MemoChatTestPostgresDaoPrivateMessagesCanRevoke(bool owner_matches,
                                                     long long existing_deleted_at_ms,
                                                     long long created_at_ms,
                                                     long long deleted_at_ms,
                                                     long long revoke_window_ms)
{
    return postgres_dao_private_messages_modules::CanRevokePrivateMessage(owner_matches,
                                                                          existing_deleted_at_ms,
                                                                          created_at_ms,
                                                                          deleted_at_ms,
                                                                          revoke_window_ms);
}

long long MemoChatTestPostgresDaoPrivateMessagesRevokeWindowStart(long long deleted_at_ms, long long revoke_window_ms)
{
    return postgres_dao_private_messages_modules::RevokeWindowStart(deleted_at_ms, revoke_window_ms);
}

bool MemoChatTestPostgresDaoPrivateMessagesCanUpsertReadState(int uid, int peer_uid)
{
    return postgres_dao_private_messages_modules::CanUpsertPrivateReadState(uid, peer_uid);
}

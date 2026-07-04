export module memochat.chat.chat_service_impl_algorithms;

export namespace memochat::chat::chat_service_impl::modules
{
// Fanout notify loop (group message / group event / group member batch) skips a
// recipient whose session lookup returned null and only counts sessions it sent
// to.
bool ShouldSkipRecipientSession(bool session_present)
{
    return !session_present;
}

// Each successfully sent recipient advances the delivered counter by one.
int NextDeliveredCount(int current, bool sent)
{
    return sent ? current + 1 : current;
}

// A fanout notify reply always reports the success error code and the number of
// sessions delivered to.
int FanoutReplyErrorCode(int success_code)
{
    return success_code;
}

// A private text-chat notify with no live session for the target reports the
// target-offline error code instead of success.
int PrivateNotifyErrorCode(bool session_present, int success_code, int target_offline_code)
{
    return session_present ? success_code : target_offline_code;
}

// A resolved sender profile only contributes a from_user_id field when the
// lookup succeeded and the public user id is non-empty.
bool ShouldAttachFromUserId(bool has_sender_info, bool user_id_empty)
{
    return has_sender_info && !user_id_empty;
}

// A cached base-info blob whose decoded public user id is empty triggers a
// Postgres refresh + Redis rewrite.
bool ShouldRefreshFromPostgres(bool user_id_empty)
{
    return user_id_empty;
}

// A per-message created_at that is non-positive falls back to the current wall
// clock timestamp.
bool ShouldUseFallbackTimestamp(long long created_at)
{
    return created_at <= 0;
}
} // namespace memochat::chat::chat_service_impl::modules

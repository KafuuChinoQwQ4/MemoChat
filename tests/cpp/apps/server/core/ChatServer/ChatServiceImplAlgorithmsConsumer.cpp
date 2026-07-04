import memochat.chat.chat_service_impl_algorithms;

namespace memochat::tests::chat::chat_service_impl
{
bool ShouldSkipRecipientSession(bool session_present)
{
    return memochat::chat::chat_service_impl::modules::ShouldSkipRecipientSession(session_present);
}

int NextDeliveredCount(int current, bool sent)
{
    return memochat::chat::chat_service_impl::modules::NextDeliveredCount(current, sent);
}

int FanoutReplyErrorCode(int success_code)
{
    return memochat::chat::chat_service_impl::modules::FanoutReplyErrorCode(success_code);
}

int PrivateNotifyErrorCode(bool session_present, int success_code, int target_offline_code)
{
    return memochat::chat::chat_service_impl::modules::PrivateNotifyErrorCode(session_present,
                                                                              success_code,
                                                                              target_offline_code);
}

bool ShouldAttachFromUserId(bool has_sender_info, bool user_id_empty)
{
    return memochat::chat::chat_service_impl::modules::ShouldAttachFromUserId(has_sender_info, user_id_empty);
}

bool ShouldRefreshFromPostgres(bool user_id_empty)
{
    return memochat::chat::chat_service_impl::modules::ShouldRefreshFromPostgres(user_id_empty);
}

bool ShouldUseFallbackTimestamp(long long created_at)
{
    return memochat::chat::chat_service_impl::modules::ShouldUseFallbackTimestamp(created_at);
}
} // namespace memochat::tests::chat::chat_service_impl

import memochat.chat.dist_lock_algorithms;

bool MemoChatTestDistLockAcquireReplyOk(int reply_type, const char* reply_text, int status_reply_type)
{
    return memochat::chat::persistence::dist_lock::modules::IsAcquireReplyOk(reply_type, reply_text, status_reply_type);
}

bool MemoChatTestDistLockReleaseReplyDeleted(int reply_type, long long integer_value, int integer_reply_type)
{
    return memochat::chat::persistence::dist_lock::modules::IsReleaseReplyDeleted(reply_type,
                                                                                  integer_value,
                                                                                  integer_reply_type);
}

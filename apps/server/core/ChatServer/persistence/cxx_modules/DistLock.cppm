export module memochat.chat.dist_lock_algorithms;

export namespace memochat::chat::persistence::dist_lock::modules
{
bool IsAcquireReplyOk(int reply_type, const char* reply_text, int status_reply_type)
{
    return reply_type == status_reply_type && reply_text != nullptr && reply_text[0] == 'O' && reply_text[1] == 'K' &&
           reply_text[2] == '\0';
}

bool IsReleaseReplyDeleted(int reply_type, long long integer_value, int integer_reply_type)
{
    return reply_type == integer_reply_type && integer_value == 1;
}
} // namespace memochat::chat::persistence::dist_lock::modules

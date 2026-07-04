export module memochat.chat.redis_mgr_algorithms;

export namespace memochat::chat::persistence::redis_mgr::modules
{
int NormalizeExpireSeconds(int expire_seconds)
{
    return expire_seconds > 0 ? expire_seconds : 1;
}

bool IsStatusOk(int reply_type, const char* reply_text, int status_reply_type)
{
    if (reply_type != status_reply_type || reply_text == nullptr)
    {
        return false;
    }
    const bool upper_ok = reply_text[0] == 'O' && reply_text[1] == 'K' && reply_text[2] == '\0';
    const bool lower_ok = reply_text[0] == 'o' && reply_text[1] == 'k' && reply_text[2] == '\0';
    return upper_ok || lower_ok;
}

bool IsStringReply(int reply_type, int string_reply_type)
{
    return reply_type == string_reply_type;
}

bool IsNilReply(int reply_type, int nil_reply_type)
{
    return reply_type == nil_reply_type;
}

bool IsIntegerReply(int reply_type, int integer_reply_type)
{
    return reply_type == integer_reply_type;
}

bool IsPositiveIntegerReply(int reply_type, long long integer_value, int integer_reply_type)
{
    return IsIntegerReply(reply_type, integer_reply_type) && integer_value > 0;
}

bool IsArrayReply(int reply_type, int array_reply_type)
{
    return reply_type == array_reply_type;
}

bool ShouldTreatEmptyLockIdentifierAsReleased(bool identifier_empty)
{
    return identifier_empty;
}
} // namespace memochat::chat::persistence::redis_mgr::modules

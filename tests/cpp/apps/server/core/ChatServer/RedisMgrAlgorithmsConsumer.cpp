import memochat.chat.redis_mgr_algorithms;

int MemoChatTestRedisMgrNormalizeExpireSeconds(int expire_seconds)
{
    return memochat::chat::persistence::redis_mgr::modules::NormalizeExpireSeconds(expire_seconds);
}

bool MemoChatTestRedisMgrStatusOk(int reply_type, const char* reply_text, int status_reply_type)
{
    return memochat::chat::persistence::redis_mgr::modules::IsStatusOk(reply_type, reply_text, status_reply_type);
}

bool MemoChatTestRedisMgrStringReply(int reply_type, int string_reply_type)
{
    return memochat::chat::persistence::redis_mgr::modules::IsStringReply(reply_type, string_reply_type);
}

bool MemoChatTestRedisMgrNilReply(int reply_type, int nil_reply_type)
{
    return memochat::chat::persistence::redis_mgr::modules::IsNilReply(reply_type, nil_reply_type);
}

bool MemoChatTestRedisMgrIntegerReply(int reply_type, int integer_reply_type)
{
    return memochat::chat::persistence::redis_mgr::modules::IsIntegerReply(reply_type, integer_reply_type);
}

bool MemoChatTestRedisMgrPositiveIntegerReply(int reply_type, long long integer_value, int integer_reply_type)
{
    return memochat::chat::persistence::redis_mgr::modules::IsPositiveIntegerReply(reply_type,
                                                                                   integer_value,
                                                                                   integer_reply_type);
}

bool MemoChatTestRedisMgrArrayReply(int reply_type, int array_reply_type)
{
    return memochat::chat::persistence::redis_mgr::modules::IsArrayReply(reply_type, array_reply_type);
}

bool MemoChatTestRedisMgrTreatEmptyLockIdentifierAsReleased(bool identifier_empty)
{
    return memochat::chat::persistence::redis_mgr::modules::ShouldTreatEmptyLockIdentifierAsReleased(identifier_empty);
}

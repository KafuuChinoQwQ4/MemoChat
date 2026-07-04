export module memochat.chat.session_repository_algorithms;

export namespace memochat::chat::persistence::session_repository::modules
{
bool ShouldAcquireDuplicateLoginLock(int uid)
{
    return uid > 0;
}

bool ShouldReleaseDuplicateLoginLock(int uid, bool lock_identifier_empty)
{
    return uid > 0 && !lock_identifier_empty;
}

bool ShouldQueryUndeliveredPrivateMessages(int uid, int limit)
{
    return uid > 0 && limit > 0;
}
} // namespace memochat::chat::persistence::session_repository::modules

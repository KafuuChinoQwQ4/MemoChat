import memochat.chat.session_repository_algorithms;

bool MemoChatTestChatSessionRepositoryShouldAcquireDuplicateLoginLock(int uid)
{
    return memochat::chat::persistence::session_repository::modules::ShouldAcquireDuplicateLoginLock(uid);
}

bool MemoChatTestChatSessionRepositoryShouldReleaseDuplicateLoginLock(int uid, bool lock_identifier_empty)
{
    return memochat::chat::persistence::session_repository::modules::ShouldReleaseDuplicateLoginLock(
        uid,
        lock_identifier_empty);
}

bool MemoChatTestChatSessionRepositoryShouldQueryUndeliveredPrivateMessages(int uid, int limit)
{
    return memochat::chat::persistence::session_repository::modules::ShouldQueryUndeliveredPrivateMessages(uid, limit);
}

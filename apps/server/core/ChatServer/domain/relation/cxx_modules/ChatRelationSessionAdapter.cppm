export module memochat.chat.relation_session_adapter_algorithms;

export namespace memochat::chat::relation_session_adapter::modules
{
// Every relation session handler is a no-op when no relation service is wired.
bool ShouldHandleCommand(bool relation_service_present)
{
    return relation_service_present;
}

// A relation command result is only sent back when a live session exists.
bool ShouldSendResult(bool session_present)
{
    return session_present;
}

// Forwarding-surface contract: the adapter forwards exactly these session
// command handlers to the relation service.
constexpr unsigned SearchUserForwardCount()
{
    return 1;
}

constexpr unsigned FriendApplyForwardCount()
{
    return 2; // add + auth
}

constexpr unsigned DeleteFriendForwardCount()
{
    return 1;
}

constexpr unsigned DialogForwardCount()
{
    return 3; // get dialog list + sync draft + pin dialog
}

constexpr unsigned ForwardingSurfaceCount()
{
    return SearchUserForwardCount() + FriendApplyForwardCount() + DeleteFriendForwardCount() +
           DialogForwardCount();
}

constexpr bool IsCompleteForwardingSurface(unsigned search_user_count,
                                           unsigned friend_apply_count,
                                           unsigned delete_friend_count,
                                           unsigned dialog_count)
{
    return search_user_count == SearchUserForwardCount() &&
           friend_apply_count == FriendApplyForwardCount() &&
           delete_friend_count == DeleteFriendForwardCount() && dialog_count == DialogForwardCount();
}
} // namespace memochat::chat::relation_session_adapter::modules

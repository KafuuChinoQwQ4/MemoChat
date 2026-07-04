import memochat.chat.relation_session_adapter_algorithms;

namespace memochat::tests::chat::relation_session_adapter
{
bool ShouldHandleCommand(bool relation_service_present)
{
    return memochat::chat::relation_session_adapter::modules::ShouldHandleCommand(relation_service_present);
}

bool ShouldSendResult(bool session_present)
{
    return memochat::chat::relation_session_adapter::modules::ShouldSendResult(session_present);
}

unsigned SearchUserForwardCount()
{
    return memochat::chat::relation_session_adapter::modules::SearchUserForwardCount();
}

unsigned FriendApplyForwardCount()
{
    return memochat::chat::relation_session_adapter::modules::FriendApplyForwardCount();
}

unsigned DeleteFriendForwardCount()
{
    return memochat::chat::relation_session_adapter::modules::DeleteFriendForwardCount();
}

unsigned DialogForwardCount()
{
    return memochat::chat::relation_session_adapter::modules::DialogForwardCount();
}

unsigned ForwardingSurfaceCount()
{
    return memochat::chat::relation_session_adapter::modules::ForwardingSurfaceCount();
}

bool IsCompleteForwardingSurface(unsigned search_user_count,
                                 unsigned friend_apply_count,
                                 unsigned delete_friend_count,
                                 unsigned dialog_count)
{
    return memochat::chat::relation_session_adapter::modules::IsCompleteForwardingSurface(search_user_count,
                                                                                          friend_apply_count,
                                                                                          delete_friend_count,
                                                                                          dialog_count);
}
} // namespace memochat::tests::chat::relation_session_adapter

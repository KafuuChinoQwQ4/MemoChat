export module memochat.chat.mongo_mgr_algorithms;

export namespace memochat::chat::persistence::mongo_mgr::modules
{
constexpr unsigned EnabledForwardCount()
{
    return 1;
}

constexpr unsigned PrivateMessageForwardCount()
{
    return 5;
}

constexpr unsigned GroupMessageForwardCount()
{
    return 5;
}

constexpr unsigned ForwardingSurfaceCount()
{
    return EnabledForwardCount() + PrivateMessageForwardCount() + GroupMessageForwardCount();
}

constexpr bool
IsCompleteForwardingSurface(unsigned enabled_count, unsigned private_message_count, unsigned group_message_count)
{
    return enabled_count == EnabledForwardCount() && private_message_count == PrivateMessageForwardCount() &&
           group_message_count == GroupMessageForwardCount();
}
} // namespace memochat::chat::persistence::mongo_mgr::modules

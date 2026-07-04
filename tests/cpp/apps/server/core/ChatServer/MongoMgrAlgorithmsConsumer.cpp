import memochat.chat.mongo_mgr_algorithms;

unsigned MemoChatTestMongoMgrEnabledForwardCount()
{
    return memochat::chat::persistence::mongo_mgr::modules::EnabledForwardCount();
}

unsigned MemoChatTestMongoMgrPrivateMessageForwardCount()
{
    return memochat::chat::persistence::mongo_mgr::modules::PrivateMessageForwardCount();
}

unsigned MemoChatTestMongoMgrGroupMessageForwardCount()
{
    return memochat::chat::persistence::mongo_mgr::modules::GroupMessageForwardCount();
}

unsigned MemoChatTestMongoMgrForwardingSurfaceCount()
{
    return memochat::chat::persistence::mongo_mgr::modules::ForwardingSurfaceCount();
}

bool MemoChatTestMongoMgrCompleteSurface(unsigned enabled_count,
                                         unsigned private_message_count,
                                         unsigned group_message_count)
{
    return memochat::chat::persistence::mongo_mgr::modules::IsCompleteForwardingSurface(enabled_count,
                                                                                        private_message_count,
                                                                                        group_message_count);
}

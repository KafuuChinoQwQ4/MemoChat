export module memochat.chat.user_support_algorithms;

export namespace memochat::chat::user_support::modules
{
bool IsAsciiDigit(unsigned int byte)
{
    return byte >= static_cast<unsigned int>('0') && byte <= static_cast<unsigned int>('9');
}

bool ShouldUseCachedProfile(bool cache_hit, bool user_id_empty)
{
    return cache_hit && !user_id_empty;
}

bool ShouldReportMissingUser(bool has_user)
{
    return !has_user;
}

int FriendApplyOffset()
{
    return 0;
}

int FriendApplyLimit()
{
    return 10;
}
} // namespace memochat::chat::user_support::modules

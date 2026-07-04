import memochat.chat.user_support_algorithms;

namespace memochat::tests::chat::user_support
{
bool IsAsciiDigit(unsigned int byte)
{
    return memochat::chat::user_support::modules::IsAsciiDigit(byte);
}

bool ShouldUseCachedProfile(bool cache_hit, bool user_id_empty)
{
    return memochat::chat::user_support::modules::ShouldUseCachedProfile(cache_hit, user_id_empty);
}

bool ShouldReportMissingUser(bool has_user)
{
    return memochat::chat::user_support::modules::ShouldReportMissingUser(has_user);
}

int FriendApplyOffset()
{
    return memochat::chat::user_support::modules::FriendApplyOffset();
}

int FriendApplyLimit()
{
    return memochat::chat::user_support::modules::FriendApplyLimit();
}
} // namespace memochat::tests::chat::user_support

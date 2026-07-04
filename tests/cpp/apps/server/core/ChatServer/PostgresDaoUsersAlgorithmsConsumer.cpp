import memochat.chat.postgres_dao_users_algorithms;

namespace postgres_dao_users_modules = memochat::chat::persistence::postgres_dao_users::modules;

bool MemoChatTestPostgresDaoUsersAsciiDigit(char ch)
{
    return postgres_dao_users_modules::IsAsciiDigit(ch);
}

bool MemoChatTestPostgresDaoUsersNonZeroAsciiDigit(char ch)
{
    return postgres_dao_users_modules::IsNonZeroAsciiDigit(ch);
}

bool MemoChatTestPostgresDaoUsersValidPublicId(const char* text, int length)
{
    return postgres_dao_users_modules::IsValidUserPublicIdShape(text, length);
}

bool MemoChatTestPostgresDaoUsersPositiveUid(int uid)
{
    return postgres_dao_users_modules::HasPositiveUid(uid);
}

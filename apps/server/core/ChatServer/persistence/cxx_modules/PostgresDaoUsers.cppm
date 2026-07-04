export module memochat.chat.postgres_dao_users_algorithms;

export namespace memochat::chat::persistence::postgres_dao_users::modules
{
bool IsAsciiDigit(char ch)
{
    return ch >= '0' && ch <= '9';
}

bool IsNonZeroAsciiDigit(char ch)
{
    return ch >= '1' && ch <= '9';
}

bool IsValidUserPublicIdShape(const char* text, int length)
{
    if (text == nullptr || length != 10)
    {
        return false;
    }
    if (text[0] != 'u' || !IsNonZeroAsciiDigit(text[1]))
    {
        return false;
    }
    for (int i = 2; i < length; ++i)
    {
        if (!IsAsciiDigit(text[i]))
        {
            return false;
        }
    }
    return true;
}

bool HasPositiveUid(int uid)
{
    return uid > 0;
}
} // namespace memochat::chat::persistence::postgres_dao_users::modules

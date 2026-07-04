export module memochat.chat.runtime_algorithms;

export namespace memochat::chat::runtime::modules
{
bool IsAsciiUpper(unsigned char ch)
{
    return ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z');
}

char ToLowerAscii(unsigned char ch)
{
    if (IsAsciiUpper(ch))
    {
        return static_cast<char>(ch - static_cast<unsigned char>('A') + static_cast<unsigned char>('a'));
    }
    return static_cast<char>(ch);
}

bool EqualsLowerAscii(const char* data, unsigned long long size, const char* literal, unsigned long long literal_size)
{
    if (data == nullptr || literal == nullptr || size != literal_size)
    {
        return false;
    }
    for (unsigned long long index = 0; index < size; ++index)
    {
        if (ToLowerAscii(static_cast<unsigned char>(data[index])) != literal[index])
        {
            return false;
        }
    }
    return true;
}

bool ParseBoolFlagOr(const char* data, unsigned long long size, bool fallback)
{
    if (data == nullptr || size == 0)
    {
        return fallback;
    }
    return EqualsLowerAscii(data, size, "1", 1) || EqualsLowerAscii(data, size, "true", 4) ||
           EqualsLowerAscii(data, size, "yes", 3) || EqualsLowerAscii(data, size, "on", 2);
}

int AtLeast(int value, int minimum)
{
    return value < minimum ? minimum : value;
}
} // namespace memochat::chat::runtime::modules

export module memochat.chat.messaging_config_algorithms;

export namespace memochat::chat::messaging::modules
{
bool IsAsciiDigit(unsigned char ch)
{
    return ch >= static_cast<unsigned char>('0') && ch <= static_cast<unsigned char>('9');
}

bool IsAsciiSpace(unsigned char ch)
{
    return ch == static_cast<unsigned char>(' ') || ch == static_cast<unsigned char>('\t') ||
           ch == static_cast<unsigned char>('\n') || ch == static_cast<unsigned char>('\r') ||
           ch == static_cast<unsigned char>('\f') || ch == static_cast<unsigned char>('\v');
}

int ParseIntOr(const char* data, unsigned long long size, int fallback)
{
    if (data == nullptr || size == 0)
    {
        return fallback;
    }

    unsigned long long index = 0;
    while (index < size && IsAsciiSpace(static_cast<unsigned char>(data[index])))
    {
        ++index;
    }
    if (index == size)
    {
        return fallback;
    }

    bool negative = false;
    if (data[index] == '+' || data[index] == '-')
    {
        negative = data[index] == '-';
        ++index;
    }
    if (index == size || !IsAsciiDigit(static_cast<unsigned char>(data[index])))
    {
        return fallback;
    }

    int value = 0;
    for (; index < size; ++index)
    {
        const auto ch = static_cast<unsigned char>(data[index]);
        if (IsAsciiSpace(ch))
        {
            return negative ? -value : value;
        }
        if (!IsAsciiDigit(ch))
        {
            return negative ? -value : value;
        }
        const int digit = static_cast<int>(ch - static_cast<unsigned char>('0'));
        if (value > (2147483647 - digit) / 10)
        {
            return fallback;
        }
        value = value * 10 + digit;
    }
    return negative ? -value : value;
}

int AtLeast(int value, int minimum)
{
    return value < minimum ? minimum : value;
}

bool EqualsLowerAscii(const char* data, unsigned long long size, const char* literal, unsigned long long literal_size)
{
    if (data == nullptr || literal == nullptr || size != literal_size)
    {
        return false;
    }
    for (unsigned long long index = 0; index < size; ++index)
    {
        unsigned char ch = static_cast<unsigned char>(data[index]);
        if (ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z'))
        {
            ch = static_cast<unsigned char>(ch - static_cast<unsigned char>('A') + static_cast<unsigned char>('a'));
        }
        if (ch != static_cast<unsigned char>(literal[index]))
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
} // namespace memochat::chat::messaging::modules

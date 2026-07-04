export module memochat.chat.config_algorithms;

export namespace memochat::chat::config::modules
{
void LowerAsciiInPlace(char* data, unsigned long long size)
{
    for (unsigned long long index = 0; index < size; ++index)
    {
        const auto ch = static_cast<unsigned char>(data[index]);
        if (ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z'))
        {
            data[index] = static_cast<char>(ch - static_cast<unsigned char>('A') + static_cast<unsigned char>('a'));
        }
    }
}

bool EqualsAscii(const char* data, unsigned long long size, const char* expected, unsigned long long expected_size)
{
    if (size != expected_size)
    {
        return false;
    }
    for (unsigned long long index = 0; index < size; ++index)
    {
        auto ch = static_cast<unsigned char>(data[index]);
        if (ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z'))
        {
            ch = static_cast<unsigned char>(ch - static_cast<unsigned char>('A') + static_cast<unsigned char>('a'));
        }
        if (static_cast<char>(ch) != expected[index])
        {
            return false;
        }
    }
    return true;
}

bool FeatureFlagDefaultTrue(const char* data, unsigned long long size)
{
    if (size == 0)
    {
        return true;
    }
    return EqualsAscii(data, size, "1", 1) || EqualsAscii(data, size, "true", 4) ||
           EqualsAscii(data, size, "yes", 3) || EqualsAscii(data, size, "on", 2);
}

int ClampInt(int value, int min_value, int max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}
} // namespace memochat::chat::config::modules

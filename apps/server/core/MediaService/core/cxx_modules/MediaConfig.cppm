export module memochat.media.config_algorithms;

export namespace memochat::media::modules
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
} // namespace memochat::media::modules

export module memochat.logging.config_algorithms;

export namespace memochat::logging::modules
{
bool IsAsciiSpace(unsigned char ch)
{
    return ch == static_cast<unsigned char>(' ') || ch == static_cast<unsigned char>('\t') ||
           ch == static_cast<unsigned char>('\n') || ch == static_cast<unsigned char>('\r') ||
           ch == static_cast<unsigned char>('\f') || ch == static_cast<unsigned char>('\v');
}

unsigned long long TrimAsciiBegin(const char* data, unsigned long long size)
{
    unsigned long long begin = 0;
    while (begin < size && IsAsciiSpace(static_cast<unsigned char>(data[begin])))
    {
        ++begin;
    }
    return begin;
}

unsigned long long TrimAsciiEnd(const char* data, unsigned long long size)
{
    unsigned long long end = size;
    while (end > 0 && IsAsciiSpace(static_cast<unsigned char>(data[end - 1])))
    {
        --end;
    }
    return end;
}

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
} // namespace memochat::logging::modules

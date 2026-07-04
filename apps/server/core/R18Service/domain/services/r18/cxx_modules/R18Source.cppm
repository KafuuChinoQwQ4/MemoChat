export module memochat.r18.source_algorithms;

export namespace memochat::r18::modules
{
bool IsAsciiAlnum(unsigned char ch)
{
    return (ch >= static_cast<unsigned char>('0') && ch <= static_cast<unsigned char>('9')) ||
           (ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z')) ||
           (ch >= static_cast<unsigned char>('a') && ch <= static_cast<unsigned char>('z'));
}

bool IsAsciiSpace(unsigned char ch)
{
    return ch == static_cast<unsigned char>(' ') || ch == static_cast<unsigned char>('\t') ||
           ch == static_cast<unsigned char>('\n') || ch == static_cast<unsigned char>('\r') ||
           ch == static_cast<unsigned char>('\f') || ch == static_cast<unsigned char>('\v');
}

char LowerAscii(unsigned char ch)
{
    if (ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z'))
    {
        return static_cast<char>(ch - static_cast<unsigned char>('A') + static_cast<unsigned char>('a'));
    }
    return static_cast<char>(ch);
}

void LowerAsciiInPlace(char* data, unsigned long long size)
{
    for (unsigned long long index = 0; index < size; ++index)
    {
        data[index] = LowerAscii(static_cast<unsigned char>(data[index]));
    }
}

bool EqualsAsciiCaseInsensitive(char left, char right)
{
    return LowerAscii(static_cast<unsigned char>(left)) == LowerAscii(static_cast<unsigned char>(right));
}

char NormalizeSourceIdChar(unsigned char ch)
{
    return IsAsciiAlnum(ch) ? LowerAscii(ch) : '-';
}

char NormalizePathSegmentChar(unsigned char ch)
{
    return IsAsciiAlnum(ch) || ch == static_cast<unsigned char>('.') || ch == static_cast<unsigned char>('_') ||
                   ch == static_cast<unsigned char>('-')
               ? static_cast<char>(ch)
               : '-';
}

bool IsPathSegmentTrimChar(char ch)
{
    return ch == '.' || ch == '-' || ch == '_';
}
} // namespace memochat::r18::modules

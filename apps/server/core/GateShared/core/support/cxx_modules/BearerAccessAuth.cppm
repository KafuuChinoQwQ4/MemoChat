export module memochat.gate.bearer_access_auth_algorithms;

export namespace memochat::gate::auth::bearer_modules
{
bool IsAsciiWhitespace(char value)
{
    return value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

char LowerAscii(char value)
{
    return value >= 'A' && value <= 'Z' ? static_cast<char>(value - 'A' + 'a') : value;
}

bool HeaderNameEquals(const char* lhs, unsigned long long lhs_size, const char* rhs, unsigned long long rhs_size)
{
    if (lhs == nullptr || rhs == nullptr || lhs_size != rhs_size)
    {
        return false;
    }
    for (unsigned long long index = 0; index < lhs_size; ++index)
    {
        if (LowerAscii(lhs[index]) != LowerAscii(rhs[index]))
        {
            return false;
        }
    }
    return true;
}

bool SchemeEqualsBearer(const char* data, unsigned long long offset, unsigned long long size)
{
    constexpr char kBearer[] = {'b', 'e', 'a', 'r', 'e', 'r'};
    if (size != 6ULL)
    {
        return false;
    }
    for (unsigned long long index = 0; index < size; ++index)
    {
        if (LowerAscii(data[offset + index]) != kBearer[index])
        {
            return false;
        }
    }
    return true;
}

bool TryLocateBearerToken(const char* data,
                          unsigned long long size,
                          unsigned long long& token_offset,
                          unsigned long long& token_size)
{
    token_offset = 0;
    token_size = 0;
    if (data == nullptr || size == 0ULL)
    {
        return false;
    }

    unsigned long long begin = 0;
    while (begin < size && IsAsciiWhitespace(data[begin]))
    {
        ++begin;
    }
    unsigned long long end = size;
    while (end > begin && IsAsciiWhitespace(data[end - 1ULL]))
    {
        --end;
    }
    if (end <= begin)
    {
        return false;
    }

    unsigned long long scheme_end = begin;
    while (scheme_end < end && !IsAsciiWhitespace(data[scheme_end]))
    {
        ++scheme_end;
    }
    if (!SchemeEqualsBearer(data, begin, scheme_end - begin))
    {
        return false;
    }
    if (scheme_end >= end || !IsAsciiWhitespace(data[scheme_end]))
    {
        return false;
    }

    unsigned long long token_begin = scheme_end;
    while (token_begin < end && IsAsciiWhitespace(data[token_begin]))
    {
        ++token_begin;
    }
    if (token_begin >= end)
    {
        return false;
    }

    token_offset = token_begin;
    token_size = end - token_begin;
    return true;
}
} // namespace memochat::gate::auth::bearer_modules

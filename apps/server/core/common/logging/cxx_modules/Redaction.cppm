export module memochat.logging.redaction_algorithms;

export namespace memochat::logging::redaction_modules
{
constexpr int kSensitiveNone = 0;
constexpr int kSensitiveSecret = 1;
constexpr int kSensitiveToken = 2;
constexpr int kSensitiveEmail = 3;
constexpr unsigned long long kTokenVisibleEdgeSize = 4;

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

bool EqualsAsciiLiteral(const char* data, unsigned long long size, const char* literal)
{
    unsigned long long index = 0;
    while (index < size && literal[index] != '\0')
    {
        if (data[index] != literal[index])
        {
            return false;
        }
        ++index;
    }
    return index == size && literal[index] == '\0';
}

bool EndsWithAsciiLiteral(const char* data, unsigned long long size, const char* literal)
{
    unsigned long long literal_size = 0;
    while (literal[literal_size] != '\0')
    {
        ++literal_size;
    }
    if (literal_size > size)
    {
        return false;
    }
    const unsigned long long offset = size - literal_size;
    for (unsigned long long index = 0; index < literal_size; ++index)
    {
        if (data[offset + index] != literal[index])
        {
            return false;
        }
    }
    return true;
}

bool ContainsAsciiLiteral(const char* data, unsigned long long size, const char* literal)
{
    unsigned long long literal_size = 0;
    while (literal[literal_size] != '\0')
    {
        ++literal_size;
    }
    if (literal_size == 0 || literal_size > size)
    {
        return false;
    }
    for (unsigned long long start = 0; start + literal_size <= size; ++start)
    {
        bool matched = true;
        for (unsigned long long index = 0; index < literal_size; ++index)
        {
            if (data[start + index] != literal[index])
            {
                matched = false;
                break;
            }
        }
        if (matched)
        {
            return true;
        }
    }
    return false;
}

int ClassifyLowerSensitiveKey(const char* key, unsigned long long size)
{
    if (EqualsAsciiLiteral(key, size, "email"))
    {
        return kSensitiveEmail;
    }
    if (EqualsAsciiLiteral(key, size, "token") || EqualsAsciiLiteral(key, size, "access_token") ||
        EqualsAsciiLiteral(key, size, "refresh_token") || EqualsAsciiLiteral(key, size, "authorization") ||
        EqualsAsciiLiteral(key, size, "cookie") || EqualsAsciiLiteral(key, size, "login_ticket") ||
        EqualsAsciiLiteral(key, size, "x-token") || EqualsAsciiLiteral(key, size, "set-cookie") ||
        EndsWithAsciiLiteral(key, size, "_token") || EndsWithAsciiLiteral(key, size, "-token"))
    {
        return kSensitiveToken;
    }
    if (EqualsAsciiLiteral(key, size, "passwd") || EqualsAsciiLiteral(key, size, "password") ||
        EqualsAsciiLiteral(key, size, "pwd") || EqualsAsciiLiteral(key, size, "password_hash") ||
        EqualsAsciiLiteral(key, size, "phone") || EqualsAsciiLiteral(key, size, "verify_code") ||
        EqualsAsciiLiteral(key, size, "api_key") || EqualsAsciiLiteral(key, size, "apikey") ||
        EqualsAsciiLiteral(key, size, "client_secret") || ContainsAsciiLiteral(key, size, "secret") ||
        EndsWithAsciiLiteral(key, size, "_password") || EndsWithAsciiLiteral(key, size, "-password") ||
        EndsWithAsciiLiteral(key, size, "_key") || EndsWithAsciiLiteral(key, size, "-key"))
    {
        return kSensitiveSecret;
    }
    return kSensitiveNone;
}

bool IsSensitiveKind(int kind)
{
    return kind != kSensitiveNone;
}

bool ShouldRedactAsEmail(int kind)
{
    return kind == kSensitiveEmail;
}

bool ShouldRedactAsToken(int kind)
{
    return kind == kSensitiveToken;
}

bool ShouldCollapseTokenMask(unsigned long long value_size)
{
    return value_size <= kTokenVisibleEdgeSize * 2ULL;
}

bool IsMaskableEmail(bool has_at, unsigned long long at_index)
{
    return has_at && at_index > 0ULL;
}

unsigned long long VisibleEmailLocalPrefix(unsigned long long local_size)
{
    return local_size <= 2ULL ? 1ULL : 2ULL;
}
} // namespace memochat::logging::redaction_modules

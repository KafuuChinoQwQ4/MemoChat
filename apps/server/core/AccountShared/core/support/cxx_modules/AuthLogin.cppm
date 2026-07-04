export module memochat.account.auth_login_algorithms;

export namespace memochat::account::auth::modules
{
constexpr int kMaxInt = 2147483647;

struct SemVerParts
{
    int major = 0;
    int minor = 0;
    int patch = 0;
};

bool IsAsciiDigit(unsigned char ch)
{
    return ch >= static_cast<unsigned char>('0') && ch <= static_cast<unsigned char>('9');
}

bool ParseSemVer(const char* data, unsigned long long size, SemVerParts& out)
{
    out = {};
    if (data == nullptr || size == 0)
    {
        return false;
    }

    int parts[3] = {0, 0, 0};
    unsigned long long part_index = 0;
    bool has_digit = false;

    for (unsigned long long index = 0; index < size; ++index)
    {
        const auto ch = static_cast<unsigned char>(data[index]);
        if (ch == static_cast<unsigned char>('.'))
        {
            if (!has_digit || part_index >= 2)
            {
                return false;
            }
            ++part_index;
            has_digit = false;
            continue;
        }
        if (!IsAsciiDigit(ch))
        {
            return false;
        }
        has_digit = true;
        const int digit = static_cast<int>(ch - static_cast<unsigned char>('0'));
        if (parts[part_index] > (kMaxInt - digit) / 10)
        {
            return false;
        }
        parts[part_index] = parts[part_index] * 10 + digit;
    }

    if (!has_digit)
    {
        return false;
    }

    out.major = parts[0];
    out.minor = parts[1];
    out.patch = parts[2];
    return true;
}

int CompareSemVer(const SemVerParts& left, const SemVerParts& right)
{
    if (left.major != right.major)
    {
        return left.major < right.major ? -1 : 1;
    }
    if (left.minor != right.minor)
    {
        return left.minor < right.minor ? -1 : 1;
    }
    if (left.patch != right.patch)
    {
        return left.patch < right.patch ? -1 : 1;
    }
    return 0;
}
} // namespace memochat::account::auth::modules

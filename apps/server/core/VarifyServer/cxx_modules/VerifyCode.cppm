export module memochat.varify.verify_code_algorithms;

export namespace memochat::varify::modules
{
char LowerAscii(char value)
{
    const auto ch = static_cast<unsigned char>(value);
    if (ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z'))
    {
        return static_cast<char>(ch - static_cast<unsigned char>('A') + static_cast<unsigned char>('a'));
    }
    return value;
}

bool EndsWithAsciiCaseInsensitive(const char* value,
                                  unsigned long long value_size,
                                  const char* suffix,
                                  unsigned long long suffix_size)
{
    if (value == nullptr || suffix == nullptr || value_size < suffix_size)
    {
        return false;
    }

    const auto start = value_size - suffix_size;
    for (unsigned long long index = 0; index < suffix_size; ++index)
    {
        if (LowerAscii(value[start + index]) != LowerAscii(suffix[index]))
        {
            return false;
        }
    }
    return true;
}

int NormalizeVerifyCodeLength(int requested_length, int default_length, int max_length)
{
    if (requested_length <= 0 || requested_length > max_length)
    {
        return default_length;
    }
    return requested_length;
}
} // namespace memochat::varify::modules

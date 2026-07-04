export module memochat.media.public_dto_algorithms;

export namespace memochat::media::public_dto::modules
{
bool ShouldUseDefaultMediaType(bool media_type_empty)
{
    return media_type_empty;
}

char LowerAsciiChar(char value)
{
    const auto ch = static_cast<unsigned char>(value);
    if (ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z'))
    {
        return static_cast<char>(ch - static_cast<unsigned char>('A') + static_cast<unsigned char>('a'));
    }
    return value;
}

bool EqualsAsciiIgnoreCaseAt(const char* data,
                             unsigned long long data_size,
                             unsigned long long offset,
                             const char* literal,
                             unsigned long long literal_size)
{
    if (data == nullptr || literal == nullptr || offset > data_size || literal_size > data_size - offset)
    {
        return false;
    }
    for (unsigned long long index = 0; index < literal_size; ++index)
    {
        if (LowerAsciiChar(data[offset + index]) != literal[index])
        {
            return false;
        }
    }
    return true;
}

bool ContainsApplicationJson(const char* data, unsigned long long data_size)
{
    constexpr const char* literal = "application/json";
    constexpr unsigned long long literal_size = 16;
    if (data == nullptr || data_size < literal_size)
    {
        return false;
    }
    for (unsigned long long offset = 0; offset + literal_size <= data_size; ++offset)
    {
        if (EqualsAsciiIgnoreCaseAt(data, data_size, offset, literal, literal_size))
        {
            return true;
        }
    }
    return false;
}
} // namespace memochat::media::public_dto::modules

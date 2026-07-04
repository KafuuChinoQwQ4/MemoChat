export module memochat.moments.public_algorithms;

export namespace memochat::moments::modules
{
constexpr int kMomentItemKindText = 0;
constexpr int kMomentItemKindImage = 1;
constexpr int kMomentItemKindVideo = 2;

bool EqualsAscii(const char* value, unsigned long long value_size, const char* literal, unsigned long long literal_size)
{
    if (value == nullptr || literal == nullptr || value_size != literal_size)
    {
        return false;
    }
    for (unsigned long long index = 0; index < value_size; ++index)
    {
        if (value[index] != literal[index])
        {
            return false;
        }
    }
    return true;
}

bool TryParseBoolText(const char* value, unsigned long long value_size, bool* output)
{
    if (output == nullptr)
    {
        return false;
    }
    if (EqualsAscii(value, value_size, "true", 4) || EqualsAscii(value, value_size, "1", 1))
    {
        *output = true;
        return true;
    }
    if (EqualsAscii(value, value_size, "false", 5) || EqualsAscii(value, value_size, "0", 1))
    {
        *output = false;
        return true;
    }
    return false;
}

int NormalizeMomentItemKind(const char* media_type,
                            unsigned long long media_type_size,
                            const char* media_key,
                            unsigned long long media_key_size)
{
    if (media_key == nullptr || media_key_size == 0)
    {
        return kMomentItemKindText;
    }
    if (EqualsAscii(media_type, media_type_size, "image", 5))
    {
        return kMomentItemKindImage;
    }
    if (EqualsAscii(media_type, media_type_size, "video", 5))
    {
        return kMomentItemKindVideo;
    }
    return kMomentItemKindText;
}

int NormalizePageLimit(int value, int default_value, int max_value)
{
    if (value <= 0)
    {
        return default_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}
} // namespace memochat::moments::modules

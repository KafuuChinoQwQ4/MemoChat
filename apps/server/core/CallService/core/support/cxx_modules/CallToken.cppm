export module memochat.call.token_algorithms;

export namespace memochat::call::modules
{
constexpr char kBase64UrlTable[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

unsigned long long Base64UrlEncodedSize(unsigned long long input_size)
{
    return (input_size * 8 + 5) / 6;
}

unsigned long long EncodeBase64Url(const unsigned char* input,
                                   unsigned long long input_size,
                                   char* output,
                                   unsigned long long output_capacity)
{
    if ((input == nullptr && input_size > 0) || output == nullptr)
    {
        return 0;
    }

    const auto required = Base64UrlEncodedSize(input_size);
    if (output_capacity < required)
    {
        return 0;
    }

    unsigned int value = 0;
    int value_bits = -6;
    unsigned long long output_size = 0;
    for (unsigned long long index = 0; index < input_size; ++index)
    {
        value = (value << 8) + input[index];
        value_bits += 8;
        while (value_bits >= 0)
        {
            output[output_size++] = kBase64UrlTable[(value >> value_bits) & 0x3F];
            value_bits -= 6;
        }
    }
    if (value_bits > -6)
    {
        output[output_size++] = kBase64UrlTable[((value << 8) >> (value_bits + 8)) & 0x3F];
    }
    return output_size;
}
} // namespace memochat::call::modules

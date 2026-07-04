import memochat.call.token_algorithms;

#include <string>

std::string MemoChatCallTestBase64UrlEncode(const std::string& input)
{
    std::string output(memochat::call::modules::Base64UrlEncodedSize(input.size()), '\0');
    const auto encoded_size =
        memochat::call::modules::EncodeBase64Url(reinterpret_cast<const unsigned char*>(input.data()),
                                                 input.size(),
                                                 output.data(),
                                                 output.size());
    if (encoded_size == 0 && !input.empty())
    {
        return {};
    }
    output.resize(encoded_size);
    return output;
}

unsigned long long MemoChatCallTestEncodeWithCapacity(const std::string& input, unsigned long long capacity)
{
    std::string output(capacity, '\0');
    return memochat::call::modules::EncodeBase64Url(reinterpret_cast<const unsigned char*>(input.data()),
                                                    input.size(),
                                                    output.data(),
                                                    output.size());
}

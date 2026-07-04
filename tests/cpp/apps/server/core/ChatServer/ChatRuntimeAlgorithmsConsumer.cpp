import memochat.chat.runtime_algorithms;

#include <string>

std::string MemoChatTestRuntimeLowerAscii(const std::string& input)
{
    std::string output;
    output.reserve(input.size());
    for (unsigned char ch : input)
    {
        output.push_back(memochat::chat::runtime::modules::ToLowerAscii(ch));
    }
    return output;
}

bool MemoChatTestRuntimeParseBoolFlagOr(const std::string& input, bool fallback)
{
    return memochat::chat::runtime::modules::ParseBoolFlagOr(input.data(), input.size(), fallback);
}

int MemoChatTestRuntimeAtLeast(int value, int minimum)
{
    return memochat::chat::runtime::modules::AtLeast(value, minimum);
}

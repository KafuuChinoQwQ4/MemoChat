import memochat.chat.messaging_config_algorithms;

#include <string>

int MemoChatTestParseMessagingIntOr(const std::string& raw, int fallback)
{
    return memochat::chat::messaging::modules::ParseIntOr(raw.data(), raw.size(), fallback);
}

int MemoChatTestMessagingAtLeast(int value, int minimum)
{
    return memochat::chat::messaging::modules::AtLeast(value, minimum);
}

bool MemoChatTestParseMessagingBoolOr(const std::string& raw, bool fallback)
{
    return memochat::chat::messaging::modules::ParseBoolFlagOr(raw.data(), raw.size(), fallback);
}

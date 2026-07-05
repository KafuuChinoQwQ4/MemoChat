#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace memochat::chatserver::transport
{

struct ChatFrame
{
    short msg_id = 0;
    std::string payload;
};

enum class ChatFrameDecodeStatus
{
    Complete,
    NeedMoreData,
    Invalid
};

class ChatFrameCodec
{
public:
    static std::optional<std::vector<std::uint8_t>> Encode(short msg_id, std::string_view payload);
    static ChatFrameDecodeStatus DecodeOne(std::vector<std::uint8_t>& buffer, ChatFrame& out);
};

} // namespace memochat::chatserver::transport

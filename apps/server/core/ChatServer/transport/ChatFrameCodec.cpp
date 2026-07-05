#include "ChatFrameCodec.hpp"

#include "const.hpp"

#include <boost/asio.hpp>

#include <cstring>
#include <limits>

namespace memochat::chatserver::transport
{

std::optional<std::vector<std::uint8_t>> ChatFrameCodec::Encode(short msg_id, std::string_view payload)
{
    if (msg_id < 0 || payload.size() > static_cast<std::size_t>(MAX_LENGTH) ||
        payload.size() > static_cast<std::size_t>(std::numeric_limits<short>::max()))
    {
        return std::nullopt;
    }

    std::vector<std::uint8_t> frame;
    frame.resize(HEAD_TOTAL_LEN + payload.size());

    const short network_id = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    const short network_len =
        boost::asio::detail::socket_ops::host_to_network_short(static_cast<short>(payload.size()));
    std::memcpy(frame.data(), &network_id, HEAD_ID_LEN);
    std::memcpy(frame.data() + HEAD_ID_LEN, &network_len, HEAD_DATA_LEN);
    if (!payload.empty())
    {
        std::memcpy(frame.data() + HEAD_TOTAL_LEN, payload.data(), payload.size());
    }

    return frame;
}

ChatFrameDecodeStatus ChatFrameCodec::DecodeOne(std::vector<std::uint8_t>& buffer, ChatFrame& out)
{
    if (buffer.size() < HEAD_TOTAL_LEN)
    {
        return ChatFrameDecodeStatus::NeedMoreData;
    }

    short msg_id = 0;
    short msg_len = 0;
    std::memcpy(&msg_id, buffer.data(), HEAD_ID_LEN);
    std::memcpy(&msg_len, buffer.data() + HEAD_ID_LEN, HEAD_DATA_LEN);
    msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
    msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);

    if (msg_id < 0 || msg_len < 0 || msg_len > MAX_LENGTH)
    {
        return ChatFrameDecodeStatus::Invalid;
    }

    const auto total_len = static_cast<std::size_t>(HEAD_TOTAL_LEN + msg_len);
    if (buffer.size() < total_len)
    {
        return ChatFrameDecodeStatus::NeedMoreData;
    }

    out.msg_id = msg_id;
    out.payload.assign(reinterpret_cast<const char*>(buffer.data() + HEAD_TOTAL_LEN),
                       static_cast<std::size_t>(msg_len));
    buffer.erase(buffer.begin(), buffer.begin() + static_cast<std::ptrdiff_t>(total_len));
    return ChatFrameDecodeStatus::Complete;
}

} // namespace memochat::chatserver::transport

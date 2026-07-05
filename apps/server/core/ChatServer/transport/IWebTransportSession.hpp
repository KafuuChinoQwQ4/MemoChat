#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace memochat::chatserver
{

using WebTransportSendFrameCallback = std::function<bool(std::vector<std::uint8_t>)>;
using WebTransportCloseCallback = std::function<void(const std::string&)>;

class IWebTransportSession
{
public:
    virtual ~IWebTransportSession() = default;

    virtual std::string sessionId() const = 0;
    virtual std::string transportName() const = 0;
    virtual void send(std::string payload, short msg_id) = 0;
    virtual void close() = 0;
    virtual bool acceptStreamBytes(std::string_view bytes) = 0;
};

} // namespace memochat::chatserver

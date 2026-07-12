#pragma once

#include "ChatSessionState.hpp"
#include "IChatSession.hpp"
#include "IWebTransportSession.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace memochat::chatserver
{

class WebTransportSession final
    : public IChatSession
    , public IWebTransportSession
    , public std::enable_shared_from_this<WebTransportSession>
{
public:
    using SendFrameCallback = WebTransportSendFrameCallback;
    using CloseCallback = WebTransportCloseCallback;
    using InboundFrameCallback = std::function<bool(const std::shared_ptr<IChatSession>&, short, std::string_view)>;
    static constexpr std::size_t kMaxDecodeBufferBytes = 10U * 1024U * 1024U;

    WebTransportSession(SendFrameCallback send_callback,
                        CloseCallback close_callback,
                        InboundFrameCallback inbound_callback = {});
    ~WebTransportSession() override;

    void Start();
    void Send(char* msg, short max_length, short msgid);
    void Send(std::string msg, short msgid);
    void Close();
    bool Ready() const noexcept;
    const std::string& startupError() const noexcept;
    std::string sessionId() const override;
    int userId() const override;
    void setUserId(int uid) override;
    std::string transportName() const override;
    void send(std::string payload, short msg_id) override;
    void close() override;
    bool isHeartbeatExpired(std::time_t now) const override;
    void updateHeartbeat() override;
    bool tryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds) override;
    void markOnlineRouteRefreshed(std::time_t now) override;

    bool AcceptStreamBytes(std::string_view bytes);
    bool acceptStreamBytes(std::string_view bytes) override;
    bool isClosed() const;

private:
    std::shared_ptr<WebTransportSession> Self();

    SendFrameCallback _send_callback;
    CloseCallback _close_callback;
    InboundFrameCallback _inbound_callback;
    ChatSessionState _state;
    std::vector<std::uint8_t> _decode_buffer;
    std::atomic_bool _closed{false};
    std::atomic_bool _close_notified{false};
};

} // namespace memochat::chatserver

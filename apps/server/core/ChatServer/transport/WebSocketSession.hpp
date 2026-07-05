#pragma once

#include "ChatSessionState.hpp"
#include "IChatSession.hpp"

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>

#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace memochat::chatserver
{

class WebSocketSession final
    : public IChatSession
    , public std::enable_shared_from_this<WebSocketSession>
{
public:
    using CloseCallback = std::function<void(const std::string&)>;
    using InboundFrameCallback = std::function<bool(const std::shared_ptr<IChatSession>&, short, std::string_view)>;

    WebSocketSession(boost::asio::ip::tcp::socket&& socket,
                     std::string path,
                     CloseCallback close_callback,
                     InboundFrameCallback inbound_callback = {});
    ~WebSocketSession() override;

    void Start();
    void Send(char* msg, short max_length, short msgid);
    void Send(std::string msg, short msgid);
    void Close();

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

private:
    static constexpr std::size_t kMaxDecodeBufferBytes = 10U * 1024U * 1024U;

    using WebSocketStream = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>;

    std::shared_ptr<WebSocketSession> Self();
    void onHttpRead(boost::beast::error_code ec);
    void onAccept(boost::beast::error_code ec);
    void doRead();
    void onRead(boost::beast::error_code ec, std::size_t bytes_transferred);
    void enqueueSend(std::vector<std::uint8_t> frame);
    void doWrite();
    void onWrite(boost::beast::error_code ec, std::size_t bytes_transferred);
    void closeOnExecutor();
    void finishClosed();
    bool pathMatches() const;

    WebSocketStream _ws;
    boost::beast::flat_buffer _buffer;
    boost::beast::http::request<boost::beast::http::string_body> _request;
    std::string _path;
    CloseCallback _close_callback;
    InboundFrameCallback _inbound_callback;
    ChatSessionState _state;
    std::deque<std::vector<std::uint8_t>> _send_queue;
    std::vector<std::uint8_t> _decode_buffer;
    std::atomic_bool _closed{false};
    std::atomic_bool _close_notified{false};
};

} // namespace memochat::chatserver

#pragma once

#include "ChatSessionState.hpp"
#include "IChatSession.hpp"

#include <boost/asio/io_context.hpp>

#include <functional>
#include <memory>

class QuicSession
    : public IChatSession
    , public std::enable_shared_from_this<QuicSession>
{
public:
    using SendCallback = std::function<bool(const std::string&, short)>;
    using CloseCallback = std::function<void(const std::string&)>;

    QuicSession(boost::asio::io_context& io_context, SendCallback send_callback, CloseCallback close_callback);
    ~QuicSession() override;

    void Start();
    void Send(std::string msg, short msgid);
    void Send(char* msg, short max_length, short msgid);
    void Close();
    bool Ready() const noexcept;
    const std::string& startupError() const noexcept;
    std::string transportName() const override;

    void HandleInboundMessage(short msgid, const std::string& payload);

    std::string sessionId() const override;
    int userId() const override;
    void setUserId(int uid) override;
    void send(std::string payload, short msg_id) override;
    void close() override;
    bool isHeartbeatExpired(std::time_t now) const override;
    void updateHeartbeat() override;
    bool tryMarkOnlineRouteRefreshDue(std::time_t now, int interval_seconds) override;
    void markOnlineRouteRefreshed(std::time_t now) override;

private:
    memochat::chatserver::ChatSessionState _state;
    SendCallback _send_callback;
    CloseCallback _close_callback;
    bool _closed = false;
};

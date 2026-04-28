#pragma once

#include "CSession.h"

#include <functional>

class QuicSession : public CSession {
public:
    using SendCallback = std::function<bool(const std::string&, short)>;
    using CloseCallback = std::function<void(const std::string&)>;

    QuicSession(boost::asio::io_context& io_context,
                SendCallback send_callback,
                CloseCallback close_callback);
    ~QuicSession() override;

    void Start() override;
    void Send(std::string msg, short msgid) override;
    void Send(char* msg, short max_length, short msgid) override;
    void Close() override;

    void HandleInboundMessage(short msgid, const std::string& payload);

private:
    SendCallback _send_callback;
    CloseCallback _close_callback;
    bool _closed = false;
};

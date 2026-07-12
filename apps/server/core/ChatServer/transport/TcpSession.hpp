#pragma once

#include "CSession.hpp"
#include "const.hpp"

#include <boost/asio.hpp>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>

using boost::asio::ip::tcp;

class TcpSession final : public CSession
{
public:
    TcpSession(boost::asio::io_context& io_context, IChatSessionHost* host);
    ~TcpSession() override;

    tcp::socket& GetSocket();
    void Start() override;
    void Send(char* msg, short max_length, short msgid) override;
    void Send(std::string msg, short msgid) override;
    void Close() override;
    std::string transportName() const override;

private:
    void ReadHeader();
    void ReadBody(short msg_id, short msg_len);
    void HandleReadFailure(const boost::system::error_code& error);
    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_self);

    tcp::socket _socket;
    char _data[MAX_LENGTH];
    std::atomic<bool> _b_close;
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::mutex _send_lock;
    std::shared_ptr<MsgNode> _recv_head_node;
    std::mutex _session_mtx;
};

#pragma once
#include <boost/asio.hpp>
#include <memory>
#include <queue>
#include <mutex>
#include "const.h"
#include "MsgNode.h"

using namespace std;
using boost::asio::ip::tcp;

class CServer;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context& io_context, CServer* server);
    ~CSession();

    tcp::socket& GetSocket();
    std::string GetSessionId();
    void Start();
    void Send(char* msg, short max_len, short msg_id);
    void Send(std::string msg, short msg_id);
    void Close();

private:
    void AsyncReadHead(int total_len);
    void AsyncReadBody(int length);
    void AsyncWrite();

    tcp::socket _socket;
    std::string _session_id;
    CServer* _server;
    bool _b_close;
    bool _b_head_parse;

    short _msg_id;
    
    // 收发相关
    std::shared_ptr<MsgNode> _recv_msg_node;
    std::shared_ptr<MsgNode> _recv_head_node;
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::mutex _send_lock;
};
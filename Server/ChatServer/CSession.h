#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <queue>
#include <mutex>
#include <string>
#include "MsgNode.h"
#include "const.h"

using boost::asio::ip::tcp;

class CServer;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context& io_context, CServer* server);
    ~CSession();
    tcp::socket& GetSocket();
    std::string& GetSessionId();
    void SetUserId(int uid);
    int GetUserId();
    void Start();
    void Send(char* msg, short max_length, short msgid); // 匹配 cpp 中的 Send 签名
    void Send(std::string msg, short msgid);
    void Close();

private:
    // 处理读写逻辑
    void AsyncReadHead(int total_len);
    void AsyncReadBody(int length);
    void AsyncWrite(); // 处理发送队列

    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_this);

    tcp::socket _socket;
    std::string _session_id;
    CServer* _server;
    bool _b_close;
    
    std::queue<std::shared_ptr<SendNode>> _send_que;
    std::mutex _send_lock;
    
    // 收包相关变量 (解决未声明标识符错误)
    std::shared_ptr<RecvNode> _recv_msg_node;
    std::shared_ptr<MsgNode> _recv_head_node;
    
    int _user_uid;
    short _msg_id;
};
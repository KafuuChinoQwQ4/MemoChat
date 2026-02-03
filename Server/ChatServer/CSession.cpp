#include "CSession.h"
#include "CServer.h"
#include <iostream>
#include <sstream>
#include <json/json.h>
#include "LogicSystem.h"
#include "ConfigMgr.h" // 确保包含配置管理

// 引入 Boost UUID 相关头文件
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

CSession::CSession(boost::asio::io_context& io_context, CServer* server)
    : _socket(io_context), _server(server), _b_close(false), _user_uid(0), _msg_id(0) {
    
    // 修复 UUID 生成
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _session_id = boost::uuids::to_string(a_uuid);
    
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession() {
    std::cout << "~CSession destruct" << std::endl;
}

tcp::socket& CSession::GetSocket() {
    return _socket;
}

std::string& CSession::GetSessionId() {
    return _session_id;
}

void CSession::SetUserId(int uid) {
    _user_uid = uid;
}

int CSession::GetUserId() {
    return _user_uid;
}

void CSession::Start() {
    AsyncReadHead(HEAD_TOTAL_LEN);
}

// 修复 Send 函数的 make_shared 参数问题
void CSession::Send(char* msg, short max_len, short msgid) {
    bool pending = false;
    // 注意：SendNode 构造函数需要复制 msg 内容，所以这里直接传指针和长度即可
    // 确保 SendNode 构造函数接受 (const char*, short, short)
    std::shared_ptr<SendNode> send_node = std::make_shared<SendNode>(msg, max_len, msgid);

    {
        std::lock_guard<std::mutex> lock(_send_lock);
        pending = !_send_que.empty();
        _send_que.push(send_node);
    }

    if (!pending) {
        AsyncWrite();
    }
}

void CSession::Send(std::string msg, short msgid) {
    Send((char*)msg.c_str(), (short)msg.length(), msgid);
}

void CSession::Close() {
    _socket.close();
    _b_close = true;
}

void CSession::AsyncReadHead(int total_len) {
    auto self = shared_from_this();
    boost::asio::async_read(_socket, 
        boost::asio::buffer(_recv_head_node->_data, total_len),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cout << "Read head error" << std::endl;
                Close();
                _server->ClearSession(_session_id);
                return;
            }

            // 解析头部
            short msg_id = 0;
            memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
            msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
            _msg_id = msg_id;

            short msg_len = 0;
            memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
            msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);

            if (msg_len > MAX_LENGTH) {
                std::cout << "Invalid body length" << std::endl;
                Close();
                _server->ClearSession(_session_id);
                return;
            }

            AsyncReadBody(msg_len);
        });
}

void CSession::AsyncReadBody(int length) {
    _recv_msg_node = std::make_shared<RecvNode>(length, _msg_id);
    auto self = shared_from_this();
    
    boost::asio::async_read(_socket,
        boost::asio::buffer(_recv_msg_node->_data, length),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cout << "Read body error" << std::endl;
                Close();
                _server->ClearSession(_session_id);
                return;
            }
            
            // 投递到逻辑系统
            LogicSystem::GetInstance()->PostMsgToQue(self, _recv_msg_node->_msg_id, 
                std::string(_recv_msg_node->_data, _recv_msg_node->_total_len));
            
            AsyncReadHead(HEAD_TOTAL_LEN);
        });
}

void CSession::AsyncWrite() {
    std::lock_guard<std::mutex> lock(_send_lock);
    if (_send_que.empty()) return;

    auto& send_node = _send_que.front();

    boost::asio::async_write(_socket,
        boost::asio::buffer(send_node->_data, send_node->_total_len),
        [this, self = shared_from_this()](boost::system::error_code ec, std::size_t length) {
            if (!ec) {
                std::lock_guard<std::mutex> lock(_send_lock);
                _send_que.pop();
                if (!_send_que.empty()) {
                    AsyncWrite();
                }
            } else {
                std::cout << "Write error: " << ec.message() << std::endl;
                Close();
                _server->ClearSession(_session_id);
            }
        });
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> shared_this) {
    if (!error) {
        std::lock_guard<std::mutex> lock(_send_lock);
        _send_que.pop();
        if (!_send_que.empty()) {
            AsyncWrite();
        }
    } else {
        std::cout << "HandleWrite error" << std::endl;
        Close();
        _server->ClearSession(_session_id);
    }
}
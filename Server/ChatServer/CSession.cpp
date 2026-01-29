#include "CSession.h"
#include "CServer.h"
#include "LogicSystem.h"
#include <iostream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

CSession::CSession(boost::asio::io_context& io_context, CServer* server)
    : _socket(io_context), _server(server), _b_close(false), _b_head_parse(false)
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _session_id = boost::uuids::to_string(a_uuid);
    _recv_msg_node = std::make_shared<MsgNode>(MAX_LENGTH);
    _recv_head_node = std::make_shared<MsgNode>(HEAD_TOTAL_LEN);
}

CSession::~CSession() {
    std::cout << "~CSession destruct" << std::endl;
}

tcp::socket& CSession::GetSocket() {
    return _socket;
}

std::string CSession::GetSessionId() {
    return _session_id;
}

void CSession::Start() {
    AsyncReadHead(HEAD_TOTAL_LEN);
}

void CSession::Send(char* msg, short max_len, short msg_id) {
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > MAX_SENDQUE) {
        std::cout << "session: " << _session_id << " send que fulled, size is " << MAX_SENDQUE << endl;
        return;
    }

    _send_que.push(make_shared<SendNode>(msg, max_len, msg_id));
    if (send_que_size > 0) {
        return;
    }
    AsyncWrite();
}

void CSession::Send(std::string msg, short msg_id) {
    Send(const_cast<char*>(msg.c_str()), msg.length(), msg_id);
}

void CSession::Close() {
    _socket.close();
    _b_close = true;
}

void CSession::AsyncReadHead(int total_len) {
    auto self = shared_from_this();
    _socket.async_read_some(boost::asio::buffer(_recv_head_node->_data + _recv_head_node->_cur_len, 
        total_len - _recv_head_node->_cur_len), 
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cout << "handle read head failed, error is " << ec.message() << endl;
                _server->ClearSession(_session_id);
                return;
            }

            if (bytes_transferred < _recv_head_node->_total_len - _recv_head_node->_cur_len) {
                _recv_head_node->_cur_len += bytes_transferred;
                AsyncReadHead(HEAD_TOTAL_LEN);
                return;
            }

            // 头部收完了，解析
            _recv_head_node->_cur_len = 0;
            short msg_id = 0;
            memcpy(&msg_id, _recv_head_node->_data, HEAD_ID_LEN);
            msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id); // 网络序转主机序
            _msg_id = msg_id;
            short msg_len = 0;
            memcpy(&msg_len, _recv_head_node->_data + HEAD_ID_LEN, HEAD_DATA_LEN);
            msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);

            std::cout << "msg_id is " << msg_id << ", msg_len is " << msg_len << endl;

            if (msg_len > MAX_LENGTH) {
                std::cout << "invalid data length is " << msg_len << endl;
                _server->ClearSession(_session_id);
                return;
            }

            AsyncReadBody(msg_len);
        });
}

void CSession::AsyncReadBody(int length) {
    auto self = shared_from_this();
    _socket.async_read_some(boost::asio::buffer(_recv_msg_node->_data + _recv_msg_node->_cur_len, 
        length - _recv_msg_node->_cur_len),
        [this, self, length](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cout << "handle read body failed, error is " << ec.message() << endl;
                _server->ClearSession(_session_id);
                return;
            }

            if (bytes_transferred < length - _recv_msg_node->_cur_len) {
                _recv_msg_node->_cur_len += bytes_transferred;
                AsyncReadBody(length);
                return;
            }

            // 包体收完，处理逻辑
            _recv_msg_node->_cur_len = 0; // 重置，准备下一次
            std::cout << "Receive Body: " << _recv_msg_node->_data << endl;
            
            LogicSystem::GetInstance()->PostMsgToQue(
                shared_from_this(), 
                _msg_id, 
                std::string(_recv_msg_node->_data, length)
            );
            // 下次读取头部
            AsyncReadHead(HEAD_TOTAL_LEN);
        });
}

void CSession::AsyncWrite() {
    auto self = shared_from_this();
    auto& send_node = _send_que.front();
    boost::asio::async_write(_socket, boost::asio::buffer(send_node->_data, send_node->_total_len),
        [this, self](boost::system::error_code ec, std::size_t bytes_transferred) {
            if (ec) {
                std::cout << "async write failed, error is " << ec.message() << endl;
                _server->ClearSession(_session_id);
                return;
            }

            std::lock_guard<std::mutex> lock(_send_lock);
            _send_que.pop();
            if (!_send_que.empty()) {
                AsyncWrite();
            }
        });
}
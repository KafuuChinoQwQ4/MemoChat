#pragma once
#include <string>
#include <cstring>
#include <iostream>
#include <boost/asio.hpp>
#include "const.h"

using namespace std;

// 消息接收节点 (定长头 + 变长体)
class MsgNode {
public:
    MsgNode(short max_len) : _total_len(max_len), _cur_len(0) {
        _data = new char[_total_len + 1]();
        _data[_total_len] = '\0';
    }

    ~MsgNode() {
        delete[] _data;
    }

    void Clear() {
        ::memset(_data, 0, _total_len);
        _cur_len = 0;
    }

    short _cur_len;
    short _total_len;
    char* _data;
};

// 2. [新增] 定义 RecvNode (接收包体) - 解决报错的关键
class RecvNode : public MsgNode {
public:
    RecvNode(short max_len, short msg_id) : MsgNode(max_len), _msg_id(msg_id) {}
    short _msg_id;
};

// 3. 定义 SendNode (发送包体)
class SendNode : public MsgNode {
public:
    SendNode(const char* msg, short max_len, short msg_id) 
        : MsgNode(max_len + HEAD_TOTAL_LEN), _msg_id(msg_id) {
        // 先写消息ID (2字节, 网络序)
        short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
        memcpy(_data, &msg_id_host, HEAD_ID_LEN);
        
        // 再写长度 (2字节, 网络序)
        short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
        memcpy(_data + HEAD_ID_LEN, &max_len_host, HEAD_DATA_LEN);
        
        // 最后写数据
        memcpy(_data + HEAD_TOTAL_LEN, msg, max_len);
    }
    short _msg_id;
};
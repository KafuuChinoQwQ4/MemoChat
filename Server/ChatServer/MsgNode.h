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

// 消息发送节点 (构造时就封包好)
class SendNode {
public:
    SendNode(const char* msg, short max_len, short msg_id) 
        : _msg_id(msg_id), _total_len(max_len + HEAD_TOTAL_LEN) 
    {
        _data = new char[_total_len + 1]();
        // 1. 写 ID (网络字节序: 大端)
        short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
        memcpy(_data, &msg_id_host, HEAD_ID_LEN);
        
        // 2. 写长度
        short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
        memcpy(_data + HEAD_ID_LEN, &max_len_host, HEAD_DATA_LEN);
        
        // 3. 写数据
        memcpy(_data + HEAD_TOTAL_LEN, msg, max_len);
        _data[_total_len] = '\0';
    }

    ~SendNode() {
        delete[] _data;
    }

    short _msg_id;
    short _total_len;
    char* _data;
};
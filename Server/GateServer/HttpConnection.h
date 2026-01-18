// D:\MemoChat\Server\HttpConnection.h
#pragma once
#include "const.h"

class LogicSystem; // 前向声明

class HttpConnection : public std::enable_shared_from_this<HttpConnection>
{
    friend class LogicSystem;
public:
    HttpConnection(tcp::socket socket);
    HttpConnection(net::io_context& ioc);
    tcp::socket& GetSocket() { return _socket; }
    void Start();

private:
    void CheckDeadline();
    void WriteResponse();
    void HandleReq();
    void PreParseGetParam(); // 解析GET参数

    tcp::socket  _socket;
    beast::flat_buffer  _buffer{ 8192 };
    http::request<http::dynamic_body> _request;
    http::response<http::dynamic_body> _response;
    net::steady_timer deadline_{
        _socket.get_executor(), std::chrono::seconds(60) };

    std::string _get_url;
    std::unordered_map<std::string, std::string> _get_params;
};


#pragma once
#include <memory>
#include <string>
#include "const.h"
#include <exec/task.hpp> // exec::task<> —— accept 循环协程返回类型

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& ioc, unsigned short& port);
    void Start();

private:
    // accept 循环协程:取代原 Start→async_accept→Start 互递归。
    // while co_await _acceptor.async_accept;accept 后 new_con->Start() 起 HTTP 连接。
    // ⚠️ self 必须是**传值协程参数**(不是首行 shared_from_this):两个调用点都用
    //    `std::make_shared<CServer>(ioc, port)->Start()`,临时 shared_ptr 在分号处即析构,
    //    早于 ioc.run();start_detached 把协程体 post 到 io_context,首行要等 run() 才跑。
    //    若首行才 shared_from_this(),那时唯一 owner 已没 → bad_weak_ptr → set_error →
    //    启动即 std::terminate。传值 self 在 Start() 内临时对象仍活时求值并存入协程帧,
    //    覆盖 spawn→首行窗口。ChatServer 的 AcceptLoop 靠外部 _tcp_server 强引用覆盖该窗口,
    //    GateShared 无此 owner,故必须传值。
    exec::task<void> AcceptLoop(std::shared_ptr<CServer> self);
    tcp::acceptor _acceptor;
    net::io_context& _ioc;
};

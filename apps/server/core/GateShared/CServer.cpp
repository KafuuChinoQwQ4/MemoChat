#include "CServer.h"
#include <iostream>
#include "HttpConnection.h"
#include "AsioIOServicePool.h"
#include "runtime/AsioCoScheduler.h" // memochat::runtime::IoContextScheduler

#include <exec/asio/use_sender.hpp>
#include <exec/start_detached.hpp>
#include <stdexec/execution.hpp>

CServer::CServer(boost::asio::io_context& ioc, unsigned short& port)
    : _acceptor(ioc, tcp::endpoint(tcp::v4(), port))
    , _ioc(ioc)
{
}

void CServer::Start()
{
    // 启动 accept 循环协程,钉在 acceptor 绑定的 _ioc(Gate 主 io_context)上。
    // self 在此处(临时 CServer 仍存活时)求值并以传值参数存入协程帧,覆盖 spawn→首行窗口。
    exec::start_detached(
        stdexec::on(memochat::runtime::IoContextScheduler{&_ioc}, AcceptLoop(shared_from_this())));
}

// accept 循环协程:取代原 Start→async_accept→{success: new_con->Start()+Start(); error: Start()} 递归。
exec::task<void> CServer::AcceptLoop(std::shared_ptr<CServer> self)
{
    using exec::asio::use_sender;

    (void) self; // 帧持有 self 续命;循环体内不再单独引用,加 (void) 防未用告警

    while (true)
    {
        // 每条新连接的 socket 绑到 AsioIOServicePool 轮询选出的池 io_context(与 acceptor 的
        // _ioc 不是同一个);HttpConnection 的读写在它自己的池 io_context 线程上跑。
        try
        {
            auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
            std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);

            try
            {
                co_await _acceptor.async_accept(new_con->GetSocket(), use_sender);
            }
            catch (const boost::system::system_error& e)
            {
                // accept 失败(非取消):对齐原回调 if(ec){Start();return;} —— acceptor 已关则退出,
                // 否则继续下一轮重试。取消(operation_aborted)走 set_stopped,不进这里(协程直接退出)。
                std::cout << "accept failed, error is " << e.what() << std::endl;
                if (!_acceptor.is_open())
                {
                    co_return;
                }
                continue;
            }

            boost::system::error_code option_ec;
            new_con->GetSocket().set_option(tcp::no_delay(true), option_ec);

            new_con->Start();
        }
        catch (const std::exception& e)
        {
            // 兜底:异常**不得**逃逸到 start_detached(否则 set_error → std::terminate)。
            // 单条连接的装配失败(GetIOService/make_shared/Start 抛 bad_alloc 等)不应拖垮整个
            // accept 循环 —— 放弃该连接,继续接受下一个(对齐原回调 catch 后 self->Start() 重试)。
            std::cout << "AcceptLoop exception is " << e.what() << std::endl;
        }
    }
}

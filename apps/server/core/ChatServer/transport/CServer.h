#pragma once
#include <boost/asio.hpp>
#include "CSession.h"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <boost/asio/steady_timer.hpp>
#include <exec/task.hpp> // exec::task<> —— accept/timer 循环协程返回类型

using boost::asio::ip::tcp;
class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& io_context, short port);
    ~CServer();
    void Start();
    void ClearSession(std::string);

    std::shared_ptr<CSession> GetSession(std::string);
    bool CheckValid(std::string);
    void StartTimer();
    void StopTimer();

private:
    // accept 循环协程:取代原 StartAccept→HandleAccept→StartAccept 递归。
    // while co_await acceptor.async_accept;accept 后 session->Start()+登记 _sessions。
    // 取消:StopTimer 的 _acceptor.cancel() → operation_aborted → set_stopped → 协程退出
    // (前提:io_context 仍在 run;若停机时已 io_context.stop(),挂起的协程在进程退出时被
    // OS 回收,真实资源由 StopTimer 同步 close/detach 释放——详见 CServer.cpp StopTimer 注释)。
    exec::task<void> AcceptLoop();
    // 心跳 timer 循环协程:取代原 on_timer 自递归 async_wait。
    // while co_await _timer.async_wait;检查心跳过期;expires_after(60s)。
    // 取消同 AcceptLoop:_timer.cancel() → set_stopped → 协程退出(同样以 io_context 在 run 为前提)。
    exec::task<void> TimerLoop();
    boost::asio::io_context& _io_context;
    short _port;
    tcp::acceptor _acceptor;
    std::unordered_map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _mutex;
    boost::asio::steady_timer _timer;
    std::atomic_bool _stopping{false};
};

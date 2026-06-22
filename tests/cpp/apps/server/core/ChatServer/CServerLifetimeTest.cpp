// CServer accept/timer 循环协程化 —— 停机/不泄漏验收(P2-3)
//
// 验证 P2 把 CServer 的 StartAccept→HandleAccept 递归 + on_timer 自递归 async_wait
// 换成 exec::task<void> AcceptLoop()/TimerLoop() + exec::start_detached 后:
//   ① StopTimer() 的 _timer.cancel() 能让挂在 co_await async_wait 上的 TimerLoop
//      优雅退出(operation_aborted → set_stopped → 栈展开 → self 释放);
//   ② 两个循环协程释放各自 self 后,CServer 析构、不泄漏。
//
// 本测试只走 TimerLoop 路径(StartTimer/StopTimer),不调 Start():AcceptLoop 首行
// 即 AsioIOServicePool::GetInstance()(惰性起 hardware_concurrency 线程池),会让单测
// 变重且依赖该单例。AcceptLoop 的取消语义已由 test_asioexec_prodpath.cpp 的
// AcceptorCancelStopsCoroutineGracefully 通用坐实(与 TimerLoop 同构),此处聚焦
// TimerLoop 的真实 CServer 集成 + 析构不泄漏。TimerLoop 的 Redis/Config 调用只在
// 60s 定时器**触发**时发生,本测试在触发前就 StopTimer,故无需任何 Docker 依赖。

#include <gtest/gtest.h>

#include "CServer.h"

#include <boost/asio.hpp>

#include <chrono>
#include <memory>
#include <thread>

namespace
{
namespace net = boost::asio;

TEST(CServerLifetime, StopTimerCancelsTimerLoopAndReleasesServer)
{
    net::io_context server_ctx; // CServer 的 acceptor + timer 绑定的 io_context
    auto work = net::make_work_guard(server_ctx);
    std::thread io_thread(
        [&]
        {
            server_ctx.run();
        });

    std::weak_ptr<CServer> weak;
    {
        // port=0 → 绑临时端口(本测试不实际接受连接,只验证 timer 循环的启动/取消)。
        auto server = std::make_shared<CServer>(server_ctx, /*port=*/0);
        weak = server;

        server->StartTimer(); // spawn TimerLoop(shared_from_this()) → co_await async_wait(首轮 60s,挂起)

        // 给 io 线程时间把 TimerLoop 挂到 async_wait 上。self 是传值协程参数,StartTimer() 在本地强
        // 引用 server 仍持有时求值 shared_from_this() 并存入协程帧,帧自 spawn 即续命(与 GateShared 统一)。
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_FALSE(weak.expired()) << "TimerLoop 协程帧应已自持 server";

        // StopTimer:_stopping=true + _timer.cancel() → operation_aborted → set_stopped →
        // TimerLoop 优雅展开 → 释放 self。_acceptor.cancel/close 无害(无 AcceptLoop 在跑)。
        server->StopTimer();

        // 离开作用域:本地强引用落,仅 TimerLoop 协程帧的 self(若已展开则随之析构)。
    }

    // 等 CServer 析构(weak 失效),最多 ~2s。
    bool released = false;
    for (int i = 0; i < 200; ++i)
    {
        if (weak.expired())
        {
            released = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(released) << "StopTimer 后 TimerLoop 未退出 / CServer 未析构 —— 协程帧泄漏了 server";

    work.reset();
    if (io_thread.joinable())
        io_thread.join();

    EXPECT_TRUE(weak.expired());
}

// 生产停机顺序验收:StopTimer() 在 io_context.stop() 之前,协程帧有机会被分发展开。
// 对应 ChatServer.cpp signal handler 的修后顺序。
TEST(CServerLifetime, ProductionShutdownOrder_StopTimerBeforeIoContextStop)
{
    net::io_context server_ctx;
    auto work = net::make_work_guard(server_ctx);
    std::thread io_thread(
        [&]
        {
            server_ctx.run();
        });

    std::weak_ptr<CServer> weak;
    {
        auto server = std::make_shared<CServer>(server_ctx, /*port=*/0);
        weak = server;
        server->StartTimer();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 生产顺序: 先 StopTimer(取消协程) 再 io_context.stop()
        server->StopTimer();
        work.reset();
        server_ctx.stop();
    }

    bool released = false;
    for (int i = 0; i < 200; ++i)
    {
        if (weak.expired())
        {
            released = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    EXPECT_TRUE(released) << "生产停机顺序下 CServer 未能正常析构";

    if (io_thread.joinable())
        io_thread.join();
}

} // namespace

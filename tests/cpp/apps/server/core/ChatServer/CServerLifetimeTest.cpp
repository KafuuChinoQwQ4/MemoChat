// CServer accept/timer 异步回调 —— 停机/不泄漏验收
//
// 验证 StopTimer() 取消 accept/timer、关闭会话后，异步回调不会继续调度，
// weak_ptr 捕获也不会阻止 CServer 析构。
//
// 本测试只走定时器路径，不调 Start()，避免启动 AsioIOServicePool。
// Redis/Config 调用只在 60s 定时器触发时发生；测试在触发前 StopTimer，
// 因此无需 Docker 依赖。

#include <gtest/gtest.h>

#include "CServer.hpp"
#include "runtime/ExplicitThread.hpp"

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
    memochat::runtime::ExplicitThread io_thread;
    std::string thread_error;
    ASSERT_TRUE(io_thread.Start(
        [&server_ctx]() noexcept
        {
            server_ctx.run();
        },
        &thread_error))
        << thread_error;

    std::weak_ptr<CServer> weak;
    {
        // port=0 → 绑临时端口(本测试不实际接受连接,只验证 timer 循环的启动/取消)。
        auto server = std::make_shared<CServer>(server_ctx, /*port=*/0);
        weak = server;

        server->StartTimer(); // 安排首轮 60s async_wait。

        // 给 io 线程时间挂起 async_wait；回调只捕获 weak_ptr。
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        EXPECT_FALSE(weak.expired()) << "测试作用域仍应持有 server";

        // StopTimer 设置 stopping 并取消 accept/timer；无 accept 回调在运行。
        server->StopTimer();

        // 离开作用域后只剩异步设施中的 weak_ptr。
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

    EXPECT_TRUE(released) << "StopTimer 后 CServer 未析构，异步回调仍持有 server";

    work.reset();
    if (io_thread.Joinable())
        ASSERT_TRUE(io_thread.Join(&thread_error)) << thread_error;

    EXPECT_TRUE(weak.expired());
}

// 生产停机顺序验收:StopTimer() 在 io_context.stop() 之前,并把 stop 推迟到后续
// io_context turn,让取消 completion 有机会分发展开协程帧。
// 对应 ChatServer.cpp signal handler 的修后顺序。
TEST(CServerLifetime, ProductionShutdownOrder_StopTimerBeforeIoContextStop)
{
    net::io_context server_ctx;
    auto work = net::make_work_guard(server_ctx);
    memochat::runtime::ExplicitThread io_thread;
    std::string thread_error;
    ASSERT_TRUE(io_thread.Start(
        [&server_ctx]() noexcept
        {
            server_ctx.run();
        },
        &thread_error))
        << thread_error;

    std::weak_ptr<CServer> weak;
    {
        auto server = std::make_shared<CServer>(server_ctx, /*port=*/0);
        weak = server;
        server->StartTimer();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 生产顺序:先 StopTimer(取消协程),再把 io_context.stop() 推迟到后续 turn。
        server->StopTimer();
        work.reset();
        net::post(server_ctx,
                  [&server_ctx]
                  {
                      net::post(server_ctx,
                                [&server_ctx]
                                {
                                    server_ctx.stop();
                                });
                  });
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

    if (io_thread.Joinable())
        ASSERT_TRUE(io_thread.Join(&thread_error)) << thread_error;
}

} // namespace

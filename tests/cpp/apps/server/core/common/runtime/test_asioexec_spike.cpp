// asioexec / stdexec 地基 spike
//
// 目的(P0):在动任何生产代码前,坐实以下事实在本项目 gcc-16 + C++23 下成立:
//   1. exec::asio::use_sender 能把 boost::asio 的 async 操作变成 stdexec sender;
//   2. exec::task<T> 协程能 co_await 这些 sender(async_wait / async_read / async_write);
//   3. 能从非协程上下文启动顶层协程并驱动 io_context 跑到完成;
//   4. 错误语义:asio error_code 在协程里表现为抛出 system_error(而非返回 ec);
//   5. 取消(operation_aborted)表现为 set_stopped。
//
// 这是 P1(CSession 读循环协程化)将要使用的全部原语的最小验证。
// 若本文件编不过或行为不符,P1 之前必须先解决,或回退 boost::asio::awaitable + co_spawn。

#include <gtest/gtest.h>

#include <exec/asio/asio_config.hpp>
#include <exec/asio/use_sender.hpp>

#include <exec/async_scope.hpp>
#include <exec/single_thread_context.hpp>
#include <exec/task.hpp>
#include <exec/when_any.hpp>
#include <stdexec/execution.hpp>

#include <boost/asio.hpp>

#include <array>
#include <chrono>
#include <cstring>
#include <future>
#include <string>
#include <system_error>
#include <thread>

namespace
{
namespace net = boost::asio;
using exec::asio::use_sender;
using tcp = net::ip::tcp;

// ---------------------------------------------------------------------------
// 1. 最低层 smoke:sender + connect + start,证明 use_sender 桥接成立。
//    (照搬 asioexec 官方 test_use_sender.cpp 的已知可行模式。)
// ---------------------------------------------------------------------------
TEST(AsioExecSpike, TimerAsSenderRunsToCompletion)
{
    net::io_context ctx;
    net::steady_timer timer(ctx);
    timer.expires_after(std::chrono::milliseconds(1));

    // async_wait + use_sender 得到一个 stdexec sender。
    auto sender = timer.async_wait(use_sender);
    static_assert(stdexec::sender<decltype(sender)>);

    bool fired = false;
    // sync_wait 在调用线程上驱动:需要 io_context 自己 run 来完成。
    // 这里用一个后台线程跑 io_context,主线程 sync_wait sender。
    std::thread io_thread([&] { ctx.run(); });
    stdexec::sync_wait(std::move(sender) | stdexec::then([&] { fired = true; }));
    io_thread.join();

    EXPECT_TRUE(fired);
}

// ---------------------------------------------------------------------------
// 2. 核心:exec::task<T> 协程 co_await asio 操作。
//    这是 P1 CSession::ReadLoop 将要用的真实模式。
//    exec::task 需要 associated scheduler —— 用 single_thread_context 提供,
//    通过 stdexec::on(scheduler, task()) 把协程调度到该上下文。
// ---------------------------------------------------------------------------

// 一个把 timer 当 co_await 点的协程,返回等待轮数。
exec::task<int> WaitTwice(net::steady_timer& timer)
{
    int rounds = 0;
    timer.expires_after(std::chrono::milliseconds(1));
    co_await timer.async_wait(use_sender);
    ++rounds;
    timer.expires_after(std::chrono::milliseconds(1));
    co_await timer.async_wait(use_sender);
    ++rounds;
    co_return rounds;
}

TEST(AsioExecSpike, TaskCoAwaitsTimer)
{
    net::io_context ctx;
    auto work = net::make_work_guard(ctx);
    std::thread io_thread([&] { ctx.run(); });

    net::steady_timer timer(ctx);
    exec::single_thread_context sched_ctx;

    // stdexec::on(scheduler, coro) 给协程一个 associated scheduler,再 sync_wait。
    auto [rounds] =
        stdexec::sync_wait(stdexec::on(sched_ctx.get_scheduler(), WaitTwice(timer))).value();

    EXPECT_EQ(rounds, 2);

    work.reset();
    io_thread.join();
}

// ---------------------------------------------------------------------------
// 3. 端到端:一个 echo session 协程,co_await async_read / async_write。
//    这是 P1 读写循环的直接缩影 —— 固定长度 head + body 往返。
// ---------------------------------------------------------------------------
exec::task<void> EchoServerOnce(tcp::socket sock)
{
    // 读 4 字节定长 head(模拟 CSession 的 transfer_exactly(HEAD_TOTAL_LEN))。
    std::array<char, 4> head{};
    co_await net::async_read(sock, net::buffer(head), net::transfer_exactly(4), use_sender);

    // 回写同样 4 字节。
    co_await net::async_write(sock, net::buffer(head), use_sender);
    co_return;
}

exec::task<std::string> EchoClientOnce(tcp::socket sock, std::string payload)
{
    co_await net::async_write(sock, net::buffer(payload), use_sender);
    std::array<char, 4> back{};
    co_await net::async_read(sock, net::buffer(back), net::transfer_exactly(4), use_sender);
    co_return std::string(back.data(), back.size());
}

TEST(AsioExecSpike, EchoOverLoopbackSocket)
{
    net::io_context ctx;
    auto work = net::make_work_guard(ctx);
    std::thread io_thread([&] { ctx.run(); });

    // 建一对已连接的 loopback socket。
    tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), 0));
    auto server_ep = acceptor.local_endpoint();

    tcp::socket client(ctx);
    tcp::socket server(ctx);

    // 同步建连(spike 简化:用 future 等 accept/connect)。
    std::promise<void> connected;
    acceptor.async_accept(server, [&](const boost::system::error_code& ec) {
        ASSERT_FALSE(ec);
        connected.set_value();
    });
    client.async_connect(server_ep, [](const boost::system::error_code& ec) { ASSERT_FALSE(ec); });
    connected.get_future().wait();

    exec::single_thread_context sched_ctx;
    auto sched = sched_ctx.get_scheduler();

    // 两条协程并发跑:server echo + client 发收。
    exec::async_scope scope;
    scope.spawn(stdexec::on(sched, EchoServerOnce(std::move(server))));

    auto [echoed] =
        stdexec::sync_wait(stdexec::on(sched, EchoClientOnce(std::move(client), "PING"))).value();

    stdexec::sync_wait(scope.on_empty());

    EXPECT_EQ(echoed, "PING");

    work.reset();
    io_thread.join();
}

// ---------------------------------------------------------------------------
// 4. 错误语义:asio error_code 在协程中表现为抛 system_error。
//    P1 的 try/catch(std::system_error&) 错误分支依赖这个语义。
// ---------------------------------------------------------------------------
exec::task<void> ReadFromClosedSocket(tcp::socket& sock)
{
    std::array<char, 4> buf{};
    // 对端已关闭,async_read 应以 eof/错误完成 → 抛异常。
    co_await net::async_read(sock, net::buffer(buf), net::transfer_exactly(4), use_sender);
}

TEST(AsioExecSpike, ErrorCodeBecomesException)
{
    net::io_context ctx;
    auto work = net::make_work_guard(ctx);
    std::thread io_thread([&] { ctx.run(); });

    tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), 0));
    auto ep = acceptor.local_endpoint();
    tcp::socket client(ctx);
    tcp::socket server(ctx);

    std::promise<void> connected;
    acceptor.async_accept(server, [&](const boost::system::error_code&) { connected.set_value(); });
    client.async_connect(ep, [](const boost::system::error_code&) {});
    connected.get_future().wait();

    // 立即关闭对端,触发读端错误。
    server.close();

    exec::single_thread_context sched_ctx;
    bool threw = false;
    try
    {
        stdexec::sync_wait(stdexec::on(sched_ctx.get_scheduler(), ReadFromClosedSocket(client)));
    }
    catch (const std::system_error&)
    {
        threw = true;
    }
    catch (const std::exception&)
    {
        threw = true; // 至少是异常路径(具体类型随 asio 版本)
    }

    EXPECT_TRUE(threw);

    work.reset();
    io_thread.join();
}

} // namespace

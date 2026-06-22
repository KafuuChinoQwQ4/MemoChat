// asioexec / stdexec 生产路径 spike(P1 命门验证)
//
// 目的:P0 的 echo spike 用 exec::single_thread_context 临时提供 scheduler —— 那是
// 另起一条线程,生产里不能这么干。CSession 是单 io_context 单线程模型:协程必须
// 全程跑在 socket 绑定的那个 io_context 线程上,co_await asio op 后续体也必须留在
// 该线程(否则就跨线程访问 socket,破坏现有的隐式单线程序列化)。
//
// 本文件验证两条候选生产路径,并用线程 ID 断言坐实"协程全程在 io_context 线程":
//   路径 A:自写 io_context → stdexec scheduler 适配器 + exec::task(sticky 亲和),
//           stdexec::on(io_sched, task()) 把协程钉在 io_context 线程。
//   路径 B:exec::basic_task<T, inline_task_context>(无亲和),续体留在 asio
//           completion 线程 —— 因 socket 绑定单一 io_context,completion 恒在该线程。
//
// 哪条通过,P1 CSession::ReadLoop 就用哪条。

#include <gtest/gtest.h>

#include <exec/asio/asio_config.hpp>
#include <exec/asio/use_sender.hpp>

#include <exec/async_scope.hpp>
#include <exec/start_detached.hpp>
#include <exec/task.hpp>
#include <stdexec/execution.hpp>

#include <boost/asio.hpp>
#include <boost/asio/steady_timer.hpp>

#include <array>
#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>

#include "runtime/AsioCoScheduler.h" // 被验证的生产设施(本 spike 即其来源,现反向消费)

namespace
{
namespace net = boost::asio;
using exec::asio::use_sender;
using tcp = net::ip::tcp;

// 生产 scheduler:直接复用 memochat::runtime::IoContextScheduler(本 spike 提炼的产物)。
using IoContextScheduler = memochat::runtime::IoContextScheduler;

static_assert(stdexec::scheduler<IoContextScheduler>);

// ---------------------------------------------------------------------------
// 路径 A:exec::task(sticky)钉在自写 io_context scheduler 上。
// 断言:协程体起点 + co_await timer 之后,都在 io_context 线程。
// ---------------------------------------------------------------------------
exec::task<int> CoroOnIoContext(net::steady_timer& timer, std::thread::id io_tid, std::atomic<int>& mismatches)
{
    if (std::this_thread::get_id() != io_tid)
        ++mismatches; // 协程启动点应已在 io_context 线程
    timer.expires_after(std::chrono::milliseconds(1));
    co_await timer.async_wait(use_sender);
    if (std::this_thread::get_id() != io_tid)
        ++mismatches; // co_await 续体应回到 io_context 线程(sticky)
    co_return 42;
}

TEST(AsioExecProdPath, PathA_TaskStickyOnIoContextScheduler)
{
    net::io_context ctx;
    auto work = net::make_work_guard(ctx);

    std::promise<std::thread::id> tid_promise;
    std::thread io_thread(
        [&]
        {
            tid_promise.set_value(std::this_thread::get_id());
            ctx.run();
        });
    const auto io_tid = tid_promise.get_future().get();

    net::steady_timer timer(ctx);
    std::atomic<int> mismatches{0};

    IoContextScheduler io_sched{&ctx};
    auto [result] = stdexec::sync_wait(stdexec::on(io_sched, CoroOnIoContext(timer, io_tid, mismatches))).value();

    EXPECT_EQ(result, 42);
    EXPECT_EQ(mismatches.load(), 0) << "协程没有全程跑在 io_context 线程上";

    work.reset();
    io_thread.join();
}

// ---------------------------------------------------------------------------
// 路径 B:exec::basic_task<T, inline_task_context>(无亲和),无需外部 scheduler。
// co_await asio op 后续体留在 completion(=io_context)线程。
// ---------------------------------------------------------------------------
template <typename T> using inline_task = exec::basic_task<T, exec::__task::inline_task_context<T>>;

inline_task<int> CoroInline(net::steady_timer& timer, std::thread::id io_tid, std::atomic<int>& mismatches)
{
    timer.expires_after(std::chrono::milliseconds(1));
    co_await timer.async_wait(use_sender);
    if (std::this_thread::get_id() != io_tid)
        ++mismatches; // 续体应在 asio completion 线程 = io_context 线程
    co_return 7;
}

TEST(AsioExecProdPath, PathB_InlineTaskNoScheduler)
{
    net::io_context ctx;
    auto work = net::make_work_guard(ctx);

    std::promise<std::thread::id> tid_promise;
    std::thread io_thread(
        [&]
        {
            tid_promise.set_value(std::this_thread::get_id());
            ctx.run();
        });
    const auto io_tid = tid_promise.get_future().get();

    net::steady_timer timer(ctx);
    std::atomic<int> mismatches{0};

    // inline_task 无亲和,可直接 sync_wait,无需 stdexec::on(sched, ...)。
    auto [result] = stdexec::sync_wait(CoroInline(timer, io_tid, mismatches)).value();

    EXPECT_EQ(result, 7);
    EXPECT_EQ(mismatches.load(), 0) << "inline_task co_await 续体没留在 io_context 线程";

    work.reset();
    io_thread.join();
}

// ---------------------------------------------------------------------------
// 路径 A 的真实读写循环:模拟 CSession::ReadLoop —— 在 io_context scheduler 上
// 跑一个 while 读循环,co_await async_read/async_write,断言全程不离 io_context 线程。
// 这是 P1 将要落地的确切形态。
// ---------------------------------------------------------------------------
exec::task<void> ReadLoopLike(tcp::socket& sock, std::thread::id io_tid, std::atomic<int>& mismatches)
{
    // ⚠️ 命门:STDEXEC_ASIO_USES_BOOST=1 下 use_sender 抛的是
    // boost::system::system_error(继承 std::runtime_error),**不是** std::system_error。
    // 二者无继承关系,catch(std::system_error&) 漏接 → 异常逃出协程 → spawn 无 error
    // 通道 → std::terminate。P1 所有错误分支必须 catch boost::system::system_error
    // 或退一步 catch std::exception。这推翻了旧 plan「catch(std::system_error&)」的写法。
    try
    {
        while (true)
        {
            std::array<char, 4> head{};
            co_await net::async_read(sock, net::buffer(head), net::transfer_exactly(4), use_sender);
            if (std::this_thread::get_id() != io_tid)
                ++mismatches;
            co_await net::async_write(sock, net::buffer(head), use_sender);
            if (std::this_thread::get_id() != io_tid)
                ++mismatches;
        }
    }
    catch (const boost::system::system_error&)
    {
        // 对端关闭 → eof → 退出循环(模拟 CSession 错误分支)。
    }
    co_return;
}

TEST(AsioExecProdPath, PathA_ReadLoopStaysOnIoContextThread)
{
    net::io_context ctx;
    auto work = net::make_work_guard(ctx);

    std::promise<std::thread::id> tid_promise;
    std::thread io_thread(
        [&]
        {
            tid_promise.set_value(std::this_thread::get_id());
            ctx.run();
        });
    const auto io_tid = tid_promise.get_future().get();

    tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), 0));
    auto ep = acceptor.local_endpoint();
    tcp::socket server(ctx);
    tcp::socket client(ctx);

    std::promise<void> connected;
    acceptor.async_accept(server,
                          [&](const boost::system::error_code&)
                          {
                              connected.set_value();
                          });
    client.async_connect(ep, [](const boost::system::error_code&) {});
    connected.get_future().wait();

    std::atomic<int> mismatches{0};

    IoContextScheduler io_sched{&ctx};
    exec::async_scope scope;
    scope.spawn(stdexec::on(io_sched, ReadLoopLike(server, io_tid, mismatches)));

    // 客户端发两轮 echo,再关闭触发 server 读循环退出。
    for (int i = 0; i < 2; ++i)
    {
        std::array<char, 4> payload{'P', 'I', 'N', 'G'};
        net::write(client, net::buffer(payload));
        std::array<char, 4> back{};
        net::read(client, net::buffer(back), net::transfer_exactly(4));
        EXPECT_EQ(std::string(back.data(), 4), "PING");
    }
    client.close();

    stdexec::sync_wait(scope.on_empty());

    EXPECT_EQ(mismatches.load(), 0) << "ReadLoop 续体离开了 io_context 线程";

    work.reset();
    io_thread.join();
}

// ---------------------------------------------------------------------------
// P2 命门:取消语义(operation_aborted → set_stopped → 协程优雅退出)。
//
// CServer 的 StopTimer 靠 _acceptor.cancel() / _timer.cancel() 优雅停机。协程化后,
// 挂在 co_await async_accept / async_wait 上的协程必须能被 cancel 干净唤醒:asio 的
// operation_aborted 经 use_sender 映射成 set_stopped(use_sender.hpp:112 已确认),
// 协程在 co_await 点 set_stopped → 栈展开(局部 RAII 析构)→ start_detached 的
// set_stopped(delete op,不 terminate)。本组测试坐实这条链,是 P2 TimerLoop/
// AcceptLoop 优雅停机的地基。
// ---------------------------------------------------------------------------

// RAII 哨兵:析构时置位,证明协程被 cancel 时栈正常展开(局部变量析构了)。
struct UnwindFlag
{
    std::atomic<bool>* unwound_;
    ~UnwindFlag()
    {
        unwound_->store(true);
    }
};

exec::task<void> TimerLoopCancellable(net::steady_timer& timer, std::atomic<int>& rounds, std::atomic<bool>& unwound)
{
    auto self_guard = UnwindFlag{&unwound}; // cancel 时应析构
    while (true)
    {
        timer.expires_after(std::chrono::seconds(3600)); // 长等,靠 cancel 唤醒
        co_await timer.async_wait(use_sender);           // cancel → operation_aborted → set_stopped
        rounds.fetch_add(1);                             // 不该到达(被 cancel)
    }
}

TEST(AsioExecProdPath, TimerCancelStopsCoroutineGracefully)
{
    net::io_context ctx;
    auto work = net::make_work_guard(ctx);
    std::thread io_thread(
        [&]
        {
            ctx.run();
        });

    net::steady_timer timer(ctx);
    std::atomic<int> rounds{0};
    std::atomic<bool> unwound{false};

    IoContextScheduler sched{&ctx};
    exec::start_detached(stdexec::on(sched, TimerLoopCancellable(timer, rounds, unwound)));

    // 让协程挂到 async_wait 上。
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // 取消:operation_aborted → set_stopped → 协程展开退出。
    net::post(ctx,
              [&]
              {
                  timer.cancel();
              });

    // 等协程展开(unwound 置位),最多 ~1s。
    bool released = false;
    for (int i = 0; i < 100; ++i)
    {
        if (unwound.load())
        {
            released = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(released) << "timer.cancel() 没让协程优雅退出(set_stopped 未触发栈展开)";
    EXPECT_EQ(rounds.load(), 0) << "async_wait 被 cancel 后不应继续循环体";

    work.reset();
    io_thread.join();
}

exec::task<void> AcceptLoopCancellable(tcp::acceptor& acceptor,
                                       net::io_context& ctx,
                                       std::atomic<int>& accepted,
                                       std::atomic<bool>& unwound)
{
    auto self_guard = UnwindFlag{&unwound};
    while (true)
    {
        tcp::socket sock(ctx);
        co_await acceptor.async_accept(sock, use_sender); // cancel → operation_aborted → set_stopped
        accepted.fetch_add(1);                            // 无连接进来时不该到达
    }
}

TEST(AsioExecProdPath, AcceptorCancelStopsCoroutineGracefully)
{
    net::io_context ctx;
    auto work = net::make_work_guard(ctx);
    std::thread io_thread(
        [&]
        {
            ctx.run();
        });

    tcp::acceptor acceptor(ctx, tcp::endpoint(tcp::v4(), 0));
    std::atomic<int> accepted{0};
    std::atomic<bool> unwound{false};

    IoContextScheduler sched{&ctx};
    exec::start_detached(stdexec::on(sched, AcceptLoopCancellable(acceptor, ctx, accepted, unwound)));

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    net::post(ctx,
              [&]
              {
                  acceptor.cancel();
              });

    bool released = false;
    for (int i = 0; i < 100; ++i)
    {
        if (unwound.load())
        {
            released = true;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    EXPECT_TRUE(released) << "acceptor.cancel() 没让 accept 协程优雅退出";
    EXPECT_EQ(accepted.load(), 0) << "无连接时 async_accept 被 cancel 不应进循环体";

    work.reset();
    io_thread.join();
}

} // namespace

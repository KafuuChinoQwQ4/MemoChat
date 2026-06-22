// CSession 读循环协程化 —— 生命周期/不泄漏验收(P1b-3)
//
// 验证 P1b 把 CSession 的 AsyncReadHead⇄AsyncReadBody 互递归回调换成
// exec::task<void> ReadLoop() + exec::start_detached 后,**对端断连时 CSession
// 析构、不泄漏**。这是 P1 验收里 ctest 无法靠 DTO 测试覆盖的那一条。
//
// 不泄漏的机理:start_detached 的 op state 自持;ReadLoop 首行 auto self=
// shared_from_this() 让协程帧持有 session;循环退出(本测试走 _server==nullptr
// 的干净分支:读到 head 后 CheckValid 短路 → Close();co_return;)后 op state
// 释放 → self 归还 → 最后一个 shared_ptr 落 → ~CSession。我们只持 weak_ptr,
// 断言它在超时内 expire,即证明析构真的发生、协程帧没把 session 续命泄漏。
//
// 之所以用 _server=nullptr 的分支:它不触达 LogicSystem/RedisMgr/UserMgr 等
// 单例(那些在 eof→DealExceptionSession 或 PostMsgToQue 路径上),使本测试无需
// 任何 Docker 依赖即可确定性运行。错误分支(eof→boost::system::system_error→
// DealExceptionSession)的释放语义由 prodpath spike 的 ReadLoopLike 已覆盖。

#include <gtest/gtest.h>

#include "CSession.h"

#include <boost/asio.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <memory>
#include <thread>

namespace
{
namespace net = boost::asio;
using tcp = net::ip::tcp;

// 建一对已连接的 loopback socket:返回 {server_side, client_side}。
std::pair<tcp::socket, tcp::socket> MakeConnectedPair(net::io_context& server_ctx, net::io_context& client_ctx)
{
    tcp::acceptor acceptor(server_ctx, tcp::endpoint(tcp::v4(), 0));
    auto ep = acceptor.local_endpoint();

    tcp::socket server(server_ctx);
    tcp::socket client(client_ctx);

    // 同步建连:client_ctx 不在本测试里 run(),用阻塞 connect;accept 用阻塞接受。
    std::thread connector(
        [&]
        {
            client.connect(ep);
        });
    acceptor.accept(server);
    connector.join();

    return {std::move(server), std::move(client)};
}

// CSession 把 socket 绑死在构造时传入的 io_context;ReadLoop 经 IoContextScheduler
// 钉在该 io_context 线程。本测试在后台线程 run 该 io_context 驱动协程。
//
// ⚠️ 关键时序:ReadLoop 的首行 shared_from_this() 不在 Start() 同步执行 ——
// start_detached(stdexec::on(sched, ReadLoop())) 把协程体 post 到 io_context 线程
// 才跑首行。生产里 CServer::HandleAccept 在 Start() 后立刻把 session 插入 _sessions
// map(强引用),所以首行 shared_from_this() 一定有 owner。本测试必须复刻这一点:
// 在协程取得自己的 self 之前,保留一个强引用,否则首行 shared_from_this() 抛
// bad_weak_ptr。我们用一个模拟 "registry" 的 shared_ptr 持有,等确认协程已接管
// (读到 head 推进一轮)后再释放,最后断言 weak 失效 = 析构发生、不泄漏。
TEST(CSessionLifetime, ReadLoopReleasesSessionOnPeerClose)
{
    net::io_context session_ctx; // CSession 绑定的 io_context
    net::io_context client_ctx;  // 仅用于建连,客户端侧

    auto [server_sock, client_sock] = MakeConnectedPair(session_ctx, client_ctx);

    auto work = net::make_work_guard(session_ctx);
    std::thread io_thread(
        [&]
        {
            session_ctx.run();
        });

    std::weak_ptr<CSession> weak;
    {
        // _server=nullptr → ReadLoop 读到 head 后 CheckValid 短路,走干净
        // Close();co_return; 分支,不触达任何业务单例。
        auto session = std::make_shared<CSession>(session_ctx, /*server=*/nullptr);
        weak = session;

        // CSession 构造时 _socket 默认构造在 session_ctx 上;把已连接的 server_sock
        // 移交给 session 的 _socket(同一 io_context,移动赋值合法)。
        session->GetSocket() = std::move(server_sock);

        session->Start(); // 内部 setsockopt + start_detached(ReadLoop)

        // 复刻生产 _sessions map 的强引用:发 head 让协程推进一轮(首行
        // shared_from_this() 此时已在 io 线程跑过,协程帧已自持 self),再释放本地强引用。
        std::array<std::uint8_t, HEAD_TOTAL_LEN> head{};
        net::write(client_sock, net::buffer(head));

        // 给 io 线程一点时间跑协程首行(取 self)+ 推进到 CheckValid 短路。
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EXPECT_FALSE(weak.expired()) << "协程帧应已自持 session";
        // 离开作用域:本地强引用落,仅协程帧的 self 续命(若它已 co_return 则随之析构)。
    }

    // 等析构发生(轮询 weak 是否 expire),最多 ~2s。
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

    EXPECT_TRUE(released) << "对端发完 head、ReadLoop 退出后 CSession 仍未析构 —— 协程帧泄漏了 session";

    work.reset();
    if (io_thread.joinable())
        io_thread.join();

    EXPECT_TRUE(weak.expired());
}

} // namespace

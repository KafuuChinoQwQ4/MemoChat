// CSession 异步读回调 —— 生命周期/不泄漏验收
//
// 验证 ReadHeader/ReadBody 回调通过 shared_ptr 保活，并在读取结束后释放，
// 使 CSession 可以析构且不会泄漏。
//
// 本测试走 _server==nullptr 的干净分支：读到 head 后校验短路并 Close；
// completion 释放 self 后，最后一个 shared_ptr 落下并触发析构。
//
// 之所以用 _server=nullptr 的分支:它不触达 LogicSystem/RedisMgr/UserMgr 等
// 单例(那些在 eof→DealExceptionSession 或 PostMsgToQue 路径上),使本测试无需
// 任何 Docker 依赖即可确定性运行；I/O 错误通过 error_code 显式处理。

#include <gtest/gtest.h>

#include "TcpSession.hpp"
#include "runtime/ExplicitThread.hpp"

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
    boost::system::error_code setup_error;
    tcp::acceptor acceptor(server_ctx);
    acceptor.open(tcp::v4(), setup_error);
    if (!setup_error)
    {
        acceptor.bind(tcp::endpoint(tcp::v4(), 0), setup_error);
    }
    if (!setup_error)
    {
        acceptor.listen(boost::asio::socket_base::max_listen_connections, setup_error);
    }
    const auto ep = setup_error ? tcp::endpoint{} : acceptor.local_endpoint(setup_error);

    tcp::socket server(server_ctx);
    tcp::socket client(client_ctx);
    if (setup_error)
    {
        ADD_FAILURE() << setup_error.message();
        return {std::move(server), std::move(client)};
    }

    // 同步建连:client_ctx 不在本测试里 run(),用阻塞 connect;accept 用阻塞接受。
    memochat::runtime::ExplicitThread connector;
    std::string thread_error;
    boost::system::error_code connect_error;
    if (!connector.Start(
            [&client, &connect_error, ep]() noexcept
            {
                client.connect(ep, connect_error);
            },
            &thread_error))
    {
        ADD_FAILURE() << thread_error;
        return {std::move(server), std::move(client)};
    }
    boost::system::error_code accept_error;
    acceptor.accept(server, accept_error);
    if (!connector.Join(&thread_error))
    {
        ADD_FAILURE() << thread_error;
    }
    if (connect_error)
    {
        ADD_FAILURE() << connect_error.message();
    }
    if (accept_error)
    {
        ADD_FAILURE() << accept_error.message();
    }

    return {std::move(server), std::move(client)};
}

// CSession 把 socket 绑定在构造时传入的 io_context；测试在后台线程运行它。
//
// Start() 同步创建第一个 async_read 并捕获 self。测试保留一个模拟 registry 的
// shared_ptr，等读取推进一轮后释放，再断言 weak 失效。
TEST(CSessionLifetime, ReadLoopReleasesSessionOnPeerClose)
{
    net::io_context session_ctx; // CSession 绑定的 io_context
    net::io_context client_ctx;  // 仅用于建连,客户端侧

    auto [server_sock, client_sock] = MakeConnectedPair(session_ctx, client_ctx);

    auto work = net::make_work_guard(session_ctx);
    memochat::runtime::ExplicitThread io_thread;
    std::string thread_error;
    ASSERT_TRUE(io_thread.Start(
        [&session_ctx]() noexcept
        {
            session_ctx.run();
        },
        &thread_error))
        << thread_error;

    std::weak_ptr<CSession> weak;
    {
        // _server=nullptr → ReadHeader 读到 head 后校验短路并 Close。
        auto session = std::make_shared<TcpSession>(session_ctx, /*server=*/nullptr);
        weak = session;

        // TcpSession 构造时 _socket 默认构造在 session_ctx 上;把已连接的 server_sock
        // 移交给 session 的 _socket(同一 io_context,移动赋值合法)。
        session->GetSocket() = std::move(server_sock);

        session->Start(); // 内部 setsockopt + ReadHeader。

        // 复刻生产 _sessions map 的强引用；发 head 让回调推进一轮。
        std::array<std::uint8_t, HEAD_TOTAL_LEN> head{};
        net::write(client_sock, net::buffer(head));

        // 给 io 线程一点时间推进到 server 校验短路。
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        EXPECT_FALSE(weak.expired()) << "测试作用域仍应持有 session";
        // 离开作用域后，完成回调应已释放它捕获的 self。
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

    EXPECT_TRUE(released) << "读回调结束后 CSession 仍未析构";

    work.reset();
    if (io_thread.Joinable())
        ASSERT_TRUE(io_thread.Join(&thread_error)) << thread_error;

    EXPECT_TRUE(weak.expired());
}

} // namespace

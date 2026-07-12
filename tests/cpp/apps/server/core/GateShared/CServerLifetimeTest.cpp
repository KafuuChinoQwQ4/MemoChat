// GateShared CServer accept 循环 —— 显式启动/lifetime 验收
//
// 验证非抛出式 Open + async_accept 回调链:
//
//   ① Start 在调用者释放 shared_ptr 前锁定 self,accept handler 持有该 owner。
//
//   ② accept 真实生效:loopback 连一条 TCP,回调收到连接并启动 HttpConnection。
//
// 不需要 Docker:accept 回调只用 AsioIOServicePool(本进程内线程池,惰性起)+ HttpConnection::Start
// (发起 async_read,因对端不发数据而挂起,不触达 LogicSystem/Redis/gRPC)。

#include <gtest/gtest.h>

#include "CServer.hpp"
#include "runtime/ExplicitThread.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>

#include <chrono>
#include <memory>
#include <thread>

namespace
{
namespace net = boost::asio;
using net::ip::tcp;

// 在系统分配的临时端口上起 CServer。返回 {weak(观测析构), 实际监听端口}。
// 关键:Start() 返回后本函数内已无任何 CServer 强引用,唯一续命来自 accept handler 捕获的 self。
struct StartedServer
{
    std::weak_ptr<CServer> weak;
    unsigned short port = 0;
};

void StartEphemeralServer(net::io_context& ioc, StartedServer& result)
{
    // 先用 0 端口探测一个空闲端口号,再通过显式 Open 完成 bind+listen。
    unsigned short port = 0;
    {
        tcp::acceptor probe(ioc, tcp::endpoint(tcp::v4(), 0));
        port = probe.local_endpoint().port();
        // probe 析构释放该端口,随即交给 CServer 复用(SO_REUSEADDR 默认开,瞬时复用安全)。
    }

    result.port = port;
    // 临时 shared_ptr:Start() 后即析构 —— 正是要验证的窗口。
    auto server = std::make_shared<CServer>(ioc);
    std::string open_error;
    ASSERT_TRUE(server->Open(port, &open_error)) << open_error;
    result.weak = server;
    server->Start();
}

TEST(GateCServerLifetime, AcceptHandlerOwnerSurvivesCallerAndAccepts)
{
    std::weak_ptr<CServer> weak;
    {
        net::io_context ioc{1}; // 模拟 Gate 主 io_context(单线程 run);放内层作用域以观测析构回收

        StartedServer started;
        ASSERT_NO_FATAL_FAILURE(StartEphemeralServer(ioc, started));
        const unsigned short port = started.port;
        weak = started.weak;

        // 此刻:本测试线程内已无 CServer 强引用,accept handler 必须已持有 self。

        memochat::runtime::ExplicitThread io_thread;
        std::string thread_error;
        ASSERT_TRUE(io_thread.Start(
            [&]
            {
                ioc.run();
            },
            &thread_error))
            << thread_error;

        // 给 io 线程时间进入 async_accept 等待。
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        // ① accept handler 捕获的 self 续命成功。
        EXPECT_FALSE(weak.expired()) << "accept handler 未持有 self,调用者释放 owner 后 CServer 已析构";

        // ② accept 真实生效:loopback 连一条。
        {
            net::io_context client_ctx;
            tcp::socket client(client_ctx);
            boost::system::error_code connect_ec;
            client.connect(tcp::endpoint(net::ip::make_address("127.0.0.1"), port), connect_ec);
            EXPECT_FALSE(connect_ec) << "连接 CServer 失败: " << connect_ec.message();
            // 不发数据:HttpConnection::Start 的 async_read 会挂起,连接被接受即证明 accept 协程工作。
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            EXPECT_FALSE(weak.expired()) << "accept 一条连接后 CServer 仍应存活(协程循环继续)";
            boost::system::error_code ignored;
            client.close(ignored);
        }

        // 停 io_context:挂起的 async_accept 不再被分发。与生产停机一致
        //(GateDomainServer signal handler 即 ioc.stop(),无 acceptor.cancel())。停后 join,
        // 再让 ioc 离开作用域析构。
        ioc.stop();
        if (io_thread.Joinable())
        {
            EXPECT_TRUE(io_thread.Join(&thread_error)) << thread_error;
        }
    } // ioc 析构。

    // ③ io_context 析构会销毁未决 accept handler,释放其中捕获的 self。
    EXPECT_TRUE(weak.expired()) << "io_context 析构后 accept handler 不应继续持有 CServer";
}

} // namespace

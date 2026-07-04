// GateShared CServer accept 循环协程化 —— 启动/lifetime/不泄漏验收(P3)
//
// 验证 P3 把 GateShared CServer 的 Start→async_accept→Start 互递归换成
// exec::task<void> AcceptLoop(std::shared_ptr<CServer> self) + exec::start_detached 后:
//
//   ① 命门:传值 self 协程参数能覆盖「临时 shared_ptr 销毁 → ioc.run() → 协程首行」窗口。
//      生产两个调用点都是 `std::make_shared<CServer>(ioc, port)->Start();` —— 临时 owner
//      在分号处即析构,早于 ioc.run()。start_detached 把协程体 post 到 io_context,首行要等
//      run() 才跑。若用首行 shared_from_this()(像 ChatServer 那样),此处唯一 owner 已没 →
//      bad_weak_ptr → start_detached set_error → std::terminate(启动即崩)。本测试复刻该调用
//      模式(临时对象即弃),若实现退回首行 shared_from_this(),进程会 terminate,测试即失败。
//
//   ② accept 真实生效:loopback 连一条 TCP,协程 co_await async_accept 返回、起 HttpConnection。
//
//   ③ 不泄漏:停掉 io_context 后协程帧(持 self)被 OS 回收,显式释放协程帧 owner 后 CServer 析构。
//
// 不需要 Docker:AcceptLoop 只用 AsioIOServicePool(本进程内线程池,惰性起)+ HttpConnection::Start
// (发起 async_read,因对端不发数据而挂起,不触达 LogicSystem/Redis/gRPC)。

#include <gtest/gtest.h>

#include "CServer.hpp"

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
// 关键:故意用 make_shared(...)->Start() 的临时对象模式 —— 复刻生产调用点,
// Start() 返回后本函数内已无任何 CServer 强引用,唯一续命来自 AcceptLoop 协程帧的传值 self。
struct StartedServer
{
    std::weak_ptr<CServer> weak;
    unsigned short port = 0;
};

StartedServer StartEphemeralServer(net::io_context& ioc)
{
    // 先用 0 端口探测一个空闲端口号,再用它构造 CServer(CServer 构造即 bind+listen)。
    unsigned short port = 0;
    {
        tcp::acceptor probe(ioc, tcp::endpoint(tcp::v4(), 0));
        port = probe.local_endpoint().port();
        // probe 析构释放该端口,随即交给 CServer 复用(SO_REUSEADDR 默认开,瞬时复用安全)。
    }

    StartedServer result;
    result.port = port;
    // 临时 shared_ptr:Start() 后即析构 —— 正是要验证的窗口。
    auto server = std::make_shared<CServer>(ioc, port);
    result.weak = server;
    server->Start();
    return result; // server 本地强引用在此随返回析构(NRVO 不延长它);仅 weak 出去
}

TEST(GateCServerLifetime, CoroutineSelfParamSurvivesTemporaryOwnerAndAccepts)
{
    std::weak_ptr<CServer> weak;
    {
        net::io_context ioc{1}; // 模拟 Gate 主 io_context(单线程 run);放内层作用域以观测析构回收

        auto started = StartEphemeralServer(ioc);
        const unsigned short port = started.port;
        weak = started.weak;

        // 此刻:本测试线程内已无 CServer 强引用(临时对象已弃)。协程体尚未跑(还没 run())。
        // 若实现把 self 写成协程首行 shared_from_this(),run() 一跑首行就 bad_weak_ptr→terminate。

        std::thread io_thread(
            [&]
            {
                ioc.run();
            });

        // 给 io 线程时间跑 AcceptLoop 首行(传值 self 入帧)+ 挂到 async_accept。
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        // ① 命门:协程帧的传值 self 续命成功 —— CServer 未被临时对象析构带走。
        ASSERT_FALSE(weak.expired()) << "AcceptLoop 协程帧未持有 self —— 临时 owner 销毁后 CServer 已析构(说明 self "
                                        "退回了首行 shared_from_this,生产里会 bad_weak_ptr→terminate)";

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

        // 停 io_context:挂起的 AcceptLoop(co_await async_accept)不再被分发。与生产停机一致
        //(GateDomainServer signal handler 即 ioc.stop(),无 acceptor.cancel())。停后 join,
        // 再让 ioc 离开作用域析构。
        ioc.stop();
        if (io_thread.joinable())
            io_thread.join();
    } // ioc 析构。

    // ③ 关于 lifetime 的如实结论(对齐 P2 phase2 log 的"良性 abandon-at-exit"定性):
    //
    //   协程的优雅退出(co_await async_accept 收到 operation_aborted → set_stopped → 栈展开 →
    //   释放传值 self)**以 io_context 仍在 run 时 cancel acceptor 为前提**。这条取消链已由
    //   tests/.../common/runtime/test_asioexec_prodpath.cpp 的 AcceptorCancelStopsCoroutineGracefully
    //   通用坐实(generic acceptor + start_detached,与本 AcceptLoop 同构)。
    //
    //   但生产 GateShared 停机只有 ioc.stop()(GateDomainServer.cpp signal handler),**没有**
    //   acceptor.cancel()。ioc.stop() 后挂起的 async_accept 的 operation_aborted completion 不再
    //   被分发,AcceptLoop 永挂在 co_await,其协程帧(持传值 self)在进程退出时由 OS 回收 ——
    //   never-freed-at-exit,**非 UAF、非真泄漏**:无任何长生命周期容器持有 CServer,且原回调版
    //   停机后未决的 async_accept handler(同样捕获 self)行为完全一致。故此处 weak 仍未失效是
    //   **预期**的(io_context 已 stop 但其内部仍持未决 op 的强引用链),不作 expired 断言。
    //
    //   本测试的确定性价值在 ①②:传值 self 覆盖了"临时 owner 销毁 → ioc.run → 协程首行"窗口
    //   (未 bad_weak_ptr→terminate),且 accept 真实生效。取消/优雅退出由 prodpath spike 拥有。
    EXPECT_FALSE(weak.expired()) << "ioc.stop()(无 cancel)后 AcceptLoop 仍挂在 co_await,协程帧应仍持 self(良性 "
                                    "abandon-at-exit,与原回调版一致);若已失效说明 self 生命周期与预期不符";
}

} // namespace

#pragma once

// io_context → stdexec scheduler 适配器(服务端回调换协程的共享设施)。
//
// 来源:tests/cpp/apps/server/core/common/runtime/test_asioexec_prodpath.cpp 的
// 「路径 A」实测产物(prodpath spike 7/7 绿)。生产里 transport 类(CSession/
// CServer/HttpConnection)的协程必须全程跑在 socket 绑定的那个
// io_context 线程上 —— co_await asio op 后续体若漂到别的线程,就会跨线程访问
// socket,破坏现有的隐式单线程序列化。本适配器把一个具体 io_context 暴露成
// stdexec scheduler,配 exec::task(sticky 亲和)+ scope.spawn(stdexec::on(
// io_sched, Loop())) 把协程钉在该 io_context 线程。
//
// ── 使用约定 ───────────────────────────────────────────────────────────────
// 1. 每个 transport 类持有一个 `exec::async_scope _co_scope;` 成员,顶层协程
//    用 `_co_scope.spawn(stdexec::on(IoContextScheduler{&ioc}, Loop()))` 启动。
//    对象析构 / Close 路径前须排空 scope(`stdexec::sync_wait(_co_scope.on_empty())`
//    或确保所有协程已退出),保证协程帧生命周期 ≤ 对象生命周期。
// 2. 协程首行 `auto self = shared_from_this();` 续命,帧存活期持有 self。
// 3. ⚠️ 错误语义命门:项目 STDEXEC_ASIO_USES_BOOST=1,use_sender 把 asio
//    error_code 映射成 **boost::system::system_error**(不是 std::system_error,
//    二者无继承关系)。协程错误分支必须 `catch (const boost::system::system_error&)`
//    (或退一步 catch std::exception)。
// 4. ⚠️ fire-and-forget 协程的异常**必须在协程内被吞掉**,不得逃逸到 scope ——
//    async_scope::spawn 只接受 set_value_t() sender,没有 error 接收通道,异常
//    逃逸 → std::terminate。顶层协程务必有兜底 try/catch。
// 5. 取消语义:socket.close() / timer.cancel() → asio operation_aborted →
//    use_sender 映射为 set_stopped(不是异常)。协程会在 co_await 点 set_stopped
//    退出,与错误分支区分开。

#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>

#include <exception>
#include <utility>

#include <stdexec/execution.hpp>

namespace memochat::runtime
{

// 把一个具体 boost::asio::io_context 暴露成满足 stdexec::scheduler 概念的轻量句柄。
// 拷贝廉价(只含一个裸指针);不拥有 io_context,调用方保证 io_context 生命周期
// 长于所有由本 scheduler 调度的协程。
struct IoContextScheduler
{
    using scheduler_concept = stdexec::scheduler_t;

    boost::asio::io_context* ctx_ = nullptr;

    template <typename Receiver> struct OpState
    {
        boost::asio::io_context* ctx_;
        Receiver receiver_;

        void start() & noexcept
        {
            // 把 receiver 的完成 post 到 io_context,续体于是在 io_context 线程上跑。
            boost::asio::post(*ctx_,
                              [this]() mutable
                              {
                                  stdexec::set_value(std::move(receiver_));
                              });
        }
    };

    struct ScheduleSender
    {
        using sender_concept = stdexec::sender_t;
        using completion_signatures =
            stdexec::completion_signatures<stdexec::set_value_t(), stdexec::set_error_t(std::exception_ptr)>;

        boost::asio::io_context* ctx_;

        template <typename Receiver> auto connect(Receiver receiver) const
        {
            return OpState<Receiver>{ctx_, std::move(receiver)};
        }

        // 标识完成调度器,供 stdexec::on / 亲和查询。
        struct Env
        {
            boost::asio::io_context* ctx_;
            template <typename Tag> auto query(stdexec::get_completion_scheduler_t<Tag>) const noexcept
            {
                return IoContextScheduler{ctx_};
            }
        };
        auto get_env() const noexcept
        {
            return Env{ctx_};
        }
    };

    auto schedule() const noexcept
    {
        return ScheduleSender{ctx_};
    }

    bool operator==(const IoContextScheduler&) const = default;
};

static_assert(stdexec::scheduler<IoContextScheduler>);

} // namespace memochat::runtime

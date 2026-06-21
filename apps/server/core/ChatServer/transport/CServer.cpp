#include "CServer.h"
#include <iostream>
#include "AsioIOServicePool.h"
#include "UserMgr.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"
#include "runtime/AsioCoScheduler.h" // memochat::runtime::IoContextScheduler

#include <exec/asio/use_sender.hpp>
#include <exec/start_detached.hpp>
#include <stdexec/execution.hpp>

CServer::CServer(boost::asio::io_context& io_context, short port)
    : _io_context(io_context)
    , _port(port)
    , _acceptor(io_context, tcp::endpoint(tcp::v4(), port))
    , _timer(_io_context, std::chrono::seconds(60))
{
    std::cout << "Server start success, listen on port : " << _port << std::endl;
}

CServer::~CServer()
{
    std::cout << "Server destruct listen on port : " << _port << std::endl;
}

void CServer::Start()
{
    // 启动 accept 循环协程,钉在 acceptor 绑定的 _io_context 上。
    // start_detached op state 自持;AcceptLoop 首行持 self 续命(ChatIngressCoordinator
    // 的 _tcp_server 强引用覆盖 spawn→首行窗口)。
    exec::start_detached(
        stdexec::on(memochat::runtime::IoContextScheduler{&_io_context}, AcceptLoop()));
}

// accept 循环协程:取代原 StartAccept→HandleAccept→StartAccept 递归。
exec::task<void> CServer::AcceptLoop()
{
    namespace net = boost::asio;
    using exec::asio::use_sender;

    auto self = shared_from_this(); // 首行续命
    while (!_stopping.load())
    {
        // 每条新连接的 socket 绑到轮询选出的 io_context(与 acceptor 的 _io_context 不必同一个);
        // session 的读循环会在它自己的 io_context 上跑(见 CSession::Start)。
        try
        {
            auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
            std::shared_ptr<CSession> new_session = std::make_shared<CSession>(io_context, this);

            try
            {
                co_await _acceptor.async_accept(new_session->GetSocket(), use_sender);
            }
            catch (const boost::system::system_error& e)
            {
                // accept 失败(非取消):对齐原 HandleAccept 的 else 分支 —— acceptor 已关则退出,
                // 否则继续下一轮重试。取消(operation_aborted)走 set_stopped,不进这里(协程直接退出)。
                std::cout << "session accept failed, error is " << e.what() << std::endl;
                if (!_acceptor.is_open())
                {
                    co_return;
                }
                continue;
            }

            if (_stopping.load())
            {
                co_return;
            }

            new_session->Start();
            {
                std::lock_guard<std::mutex> lock(_mutex);
                _sessions.insert(std::make_pair(new_session->GetSessionId(), new_session));
            }
        }
        catch (const std::exception& e)
        {
            // 兜底:异常**不得**逃逸到 start_detached(否则 set_error → std::terminate)。
            // 单条连接的装配失败(make_shared/Start/insert 抛 bad_alloc 等)不应拖垮整个
            // accept 循环 —— 放弃该连接,继续接受下一个(优于原回调版让异常逃出 io_context::run)。
            std::cout << "AcceptLoop exception is " << e.what() << std::endl;
        }
    }
}

void CServer::ClearSession(std::string session_id)
{
    std::lock_guard<std::mutex> lock(_mutex);
    if (_sessions.find(session_id) != _sessions.end())
    {
        auto uid = _sessions[session_id]->GetUserId();

        UserMgr::GetInstance()->RmvUserSession(uid, session_id);
    }

    _sessions.erase(session_id);
}

std::shared_ptr<CSession> CServer::GetSession(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(uuid);
    if (it != _sessions.end())
    {
        return it->second;
    }
    return nullptr;
}

bool CServer::CheckValid(std::string uuid)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _sessions.find(uuid);
    if (it != _sessions.end())
    {
        return true;
    }
    return false;
}

void CServer::StartTimer()
{
    // 启动心跳 timer 循环协程,钉在 timer 绑定的 _io_context 上。
    exec::start_detached(
        stdexec::on(memochat::runtime::IoContextScheduler{&_io_context}, TimerLoop()));
}

// 心跳 timer 循环协程:取代原 on_timer 自递归 async_wait。
// 首轮 co_await 等到构造时设的 60s 到期;每轮处理完 expires_after(60s) 续约。
exec::task<void> CServer::TimerLoop()
{
    namespace net = boost::asio;
    using exec::asio::use_sender;

    auto self = shared_from_this(); // 首行续命
    while (!_stopping.load())
    {
        try
        {
            // 取消(StopTimer 的 _timer.cancel())→ operation_aborted → set_stopped → 协程退出
            //(set_stopped 不是异常,不会被下面的 catch 捕获,直接结束协程)。
            co_await _timer.async_wait(use_sender);
        }
        catch (const boost::system::system_error& e)
        {
            // timer 错误(非取消):对齐原 on_timer 的 if(ec){log;return;} —— 记录并退出循环。
            std::cout << "timer error: " << e.what() << std::endl;
            co_return;
        }

        if (_stopping.load())
        {
            co_return;
        }

        // 一轮心跳巡检 + Redis 计数。这里的 Redis/Config 调用可能抛 std::exception
        // (连接抖动、bad_alloc 等);**必须**兜住,否则逃逸 start_detached → std::terminate。
        // 兜住后续约下一轮,保持心跳循环存活(优于原 on_timer 让异常逃出 io_context::run)。
        try
        {
            std::vector<std::pair<std::string, std::weak_ptr<CSession>>> expired;
            int session_count = 0;
            {
                std::lock_guard<std::mutex> lock(_mutex);
                for (const auto& [id, session_sp] : _sessions)
                {
                    time_t now = std::time(nullptr);
                    if (session_sp->IsHeartbeatExpired(now))
                    {
                        expired.emplace_back(id, session_sp);
                    }
                    else
                    {
                        session_count++;
                    }
                }
            }

            for (auto& [id, wp] : expired)
            {
                if (auto sp = wp.lock())
                {
                    sp->Close();
                    sp->DealExceptionSession();
                }
            }

            auto& cfg = ConfigMgr::Inst();
            auto self_name = cfg["SelfServer"]["Name"];
            auto count_str = std::to_string(session_count);
            RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, count_str);
        }
        catch (const std::exception& e)
        {
            std::cout << "TimerLoop tick exception is " << e.what() << std::endl;
        }

        _timer.expires_after(std::chrono::seconds(60)); // 续约下一轮(异常路径也续约,保持心跳存活)
    }
}

void CServer::StopTimer()
{
    // 停机:置 _stopping + 取消 acceptor/timer + 同步释放真实资源(关 acceptor fd、detach/close
    // 所有 session)。
    // ⚠️ 协程优雅退出的前提是 io_context 仍在 run —— cancel() 把 operation_aborted 作为 completion
    // 投递回 io_context,由 run 中的线程跑出 set_stopped 让 AcceptLoop/TimerLoop 展开退出。
    // 但生产停机顺序(app/ChatServer.cpp:signal handler 先 io_context.stop(),run() 返回后才调
    // 本函数)下 io_context 已停,该 completion 不再被分发,两个协程帧在进程退出时由 OS 回收
    // (良性,与原回调版停机后未决 handler 同样被弃)。真正要紧的资源(监听 fd、session 连接)
    // 在下方同步关闭,不依赖协程退出。若将来要做"协程全部排空再退"的优雅停机,需把 cancel 改到
    // io_context.stop() 之前、于 io 线程发起(见 .ai/callback-to-coroutine/a/logs/phase2-cserver.result.md)。
    _stopping.store(true);
    boost::system::error_code ignored;
    _timer.cancel();
    _acceptor.cancel(ignored);
    _acceptor.close(ignored);

    std::vector<std::shared_ptr<CSession>> sessions;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (auto& [id, session] : _sessions)
        {
            sessions.push_back(session);
        }
        _sessions.clear();
    }

    for (auto& session : sessions)
    {
        session->DetachServer();
        session->Close();
    }
}

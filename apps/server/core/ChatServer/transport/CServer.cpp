#include "CServer.hpp"
#include <iostream>
#include "AsioIOServicePool.hpp"
#include "TcpSession.hpp"
#include "UserMgr.hpp"
#include "RedisMgr.hpp"
#include "ConfigMgr.hpp"
#include "runtime/AsioCoScheduler.hpp" // memochat::runtime::IoContextScheduler

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
    exec::start_detached(
        stdexec::on(memochat::runtime::IoContextScheduler{&_io_context}, AcceptLoop(shared_from_this())));
}

exec::task<void> CServer::AcceptLoop(std::shared_ptr<CServer> self)
{
    namespace net = boost::asio;
    using exec::asio::use_sender;

    (void) self;
    while (!_stopping.load())
    {
        try
        {
            auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
            auto new_session = std::make_shared<TcpSession>(io_context, this);

            try
            {
                co_await _acceptor.async_accept(new_session->GetSocket(), use_sender);
            }
            catch (const boost::system::system_error& e)
            {
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
    exec::start_detached(
        stdexec::on(memochat::runtime::IoContextScheduler{&_io_context}, TimerLoop(shared_from_this())));
}

exec::task<void> CServer::TimerLoop(std::shared_ptr<CServer> self)
{
    namespace net = boost::asio;
    using exec::asio::use_sender;

    (void) self; // 帧持有 self 续命(传值参数)
    while (!_stopping.load())
    {
        try
        {
            co_await _timer.async_wait(use_sender);
        }
        catch (const boost::system::system_error& e)
        {
            // timer 错误(非取消):记录并退出。
            std::cout << "timer error: " << e.what() << std::endl;
            co_return;
        }

        if (_stopping.load())
        {
            co_return;
        }

        // 一轮心跳巡检 + Redis 计数。
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

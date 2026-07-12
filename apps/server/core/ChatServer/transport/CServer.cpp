#include "CServer.hpp"
#include <iostream>
#include "AsioIOServicePool.hpp"
#include "TcpSession.hpp"
#include "UserMgr.hpp"
#include "RedisMgr.hpp"
#include "ConfigMgr.hpp"

CServer::CServer(boost::asio::io_context& io_context, unsigned short port)
    : _io_context(io_context)
    , _port(port)
    , _acceptor(io_context)
    , _timer(_io_context)
{
    boost::system::error_code error;
    const tcp::endpoint endpoint(tcp::v4(), port);
    _acceptor.open(endpoint.protocol(), error);
    if (!error)
    {
        _acceptor.set_option(tcp::acceptor::reuse_address(true), error);
    }
    if (!error)
    {
        _acceptor.bind(endpoint, error);
    }
    if (!error)
    {
        _acceptor.listen(boost::asio::socket_base::max_listen_connections, error);
    }
    if (error)
    {
        _startup_error = error.message();
        return;
    }
    std::cout << "Server start success, listen on port : " << _port << std::endl;
}

CServer::~CServer()
{
    std::cout << "Server destruct listen on port : " << _port << std::endl;
}

bool CServer::Ready() const
{
    return _startup_error.empty() && _acceptor.is_open();
}

const std::string& CServer::startupError() const
{
    return _startup_error;
}

void CServer::Start()
{
    if (Ready())
    {
        AcceptNext();
    }
}

void CServer::AcceptNext()
{
    if (_stopping.load() || !_acceptor.is_open())
    {
        return;
    }
    auto self = weak_from_this().lock();
    if (!self)
    {
        return;
    }
    auto& session_context = AsioIOServicePool::GetInstance()->GetIOService();
    auto new_session = std::make_shared<TcpSession>(session_context, this);
    if (!new_session->Ready())
    {
        _startup_error = "session UUID generation failed: " + new_session->startupError();
        _stopping.store(true);
        boost::system::error_code ignored;
        _acceptor.close(ignored);
        std::cerr << _startup_error << std::endl;
        return;
    }
    _acceptor.async_accept(
        new_session->GetSocket(),
        [self = std::move(self), new_session = std::move(new_session)](const boost::system::error_code& error)
        {
            if (error)
            {
                if (error != boost::asio::error::operation_aborted && self->_acceptor.is_open())
                {
                    std::cout << "session accept failed, error is " << error.message() << std::endl;
                    self->AcceptNext();
                }
                return;
            }
            if (self->_stopping.load())
            {
                return;
            }
            {
                std::lock_guard<std::mutex> lock(self->_mutex);
                self->_sessions.insert(std::make_pair(new_session->GetSessionId(), new_session));
            }
            new_session->Start();
            self->AcceptNext();
        });
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
    ScheduleTimer();
}

void CServer::ScheduleTimer()
{
    if (_stopping.load())
    {
        return;
    }
    const auto weak_self = weak_from_this();
    if (weak_self.expired())
    {
        return;
    }
    _timer.expires_after(std::chrono::seconds(60));
    _timer.async_wait(
        [weak_self](const boost::system::error_code& error)
        {
            const auto self = weak_self.lock();
            if (!self)
            {
                return;
            }
            if (error)
            {
                if (error != boost::asio::error::operation_aborted)
                {
                    std::cout << "timer error: " << error.message() << std::endl;
                }
                return;
            }
            if (!self->_stopping.load())
            {
                self->RunTimerTick();
                self->ScheduleTimer();
            }
        });
}

void CServer::RunTimerTick()
{
    std::vector<std::pair<std::string, std::weak_ptr<CSession>>> expired;
    int session_count = 0;
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto& [id, session_sp] : _sessions)
        {
            const std::time_t now = std::time(nullptr);
            if (session_sp->IsHeartbeatExpired(now))
            {
                expired.emplace_back(id, session_sp);
            }
            else
            {
                ++session_count;
            }
        }
    }

    for (auto& expired_entry : expired)
    {
        if (auto session = expired_entry.second.lock())
        {
            session->Close();
            session->DealExceptionSession();
        }
    }

    auto& cfg = ConfigMgr::Inst();
    const auto self_name = cfg["SelfServer"]["Name"];
    RedisMgr::GetInstance()->HSet(LOGIN_COUNT, self_name, std::to_string(session_count));
}

void CServer::StopTimer()
{
    _stopping.store(true);
    boost::system::error_code ignored;
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

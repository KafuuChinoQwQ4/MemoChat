#pragma once
#include <boost/asio.hpp>
#include "CSession.hpp"
#include "IChatSessionHost.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <boost/asio/steady_timer.hpp>

using boost::asio::ip::tcp;
class CServer
    : public std::enable_shared_from_this<CServer>
    , public IChatSessionHost
{
public:
    CServer(boost::asio::io_context& io_context, unsigned short port);
    ~CServer();
    bool Ready() const;
    const std::string& startupError() const;
    void Start();
    void ClearSession(std::string) override;

    std::shared_ptr<CSession> GetSession(std::string);
    bool CheckValid(std::string) override;
    void StartTimer();
    void StopTimer();

private:
    void AcceptNext();
    void ScheduleTimer();
    void RunTimerTick();
    boost::asio::io_context& _io_context;
    unsigned short _port;
    tcp::acceptor _acceptor;
    std::unordered_map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _mutex;
    boost::asio::steady_timer _timer;
    std::atomic_bool _stopping{false};
    std::string _startup_error;
};

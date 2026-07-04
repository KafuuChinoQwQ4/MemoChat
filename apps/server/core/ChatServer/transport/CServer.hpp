#pragma once
#include <boost/asio.hpp>
#include "CSession.hpp"
#include <memory>
#include <unordered_map>
#include <mutex>
#include <atomic>
#include <boost/asio/steady_timer.hpp>
#include <exec/task.hpp> // exec::task<> —— accept/timer 循环协程返回类型

using boost::asio::ip::tcp;
class CServer : public std::enable_shared_from_this<CServer>
{
public:
    CServer(boost::asio::io_context& io_context, short port);
    ~CServer();
    void Start();
    void ClearSession(std::string);

    std::shared_ptr<CSession> GetSession(std::string);
    bool CheckValid(std::string);
    void StartTimer();
    void StopTimer();

private:
    exec::task<void> AcceptLoop(std::shared_ptr<CServer> self); // self 传值覆盖 spawn→首行窗口
    exec::task<void> TimerLoop(std::shared_ptr<CServer> self);  // self 传值覆盖 spawn→首行窗口
    boost::asio::io_context& _io_context;
    short _port;
    tcp::acceptor _acceptor;
    std::unordered_map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _mutex;
    boost::asio::steady_timer _timer;
    std::atomic_bool _stopping{false};
};

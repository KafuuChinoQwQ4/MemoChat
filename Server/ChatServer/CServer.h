#pragma once
#include <boost/asio.hpp>
#include <map>
#include <mutex>
#include <memory>
#include "CSession.h"

using namespace std;
using boost::asio::ip::tcp;

class CServer
{
public:
    CServer(boost::asio::io_context& io_context, short port);
    ~CServer();
    void ClearSession(std::string session_id);

private:
    void AsyncAccept();

    boost::asio::io_context& _io_context;
    tcp::acceptor _acceptor;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _mutex;
};
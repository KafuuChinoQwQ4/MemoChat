#pragma once
#include <memory>
#include <string>
#include "const.hpp"

class CServer : public std::enable_shared_from_this<CServer>
{
public:
    explicit CServer(boost::asio::io_context& ioc);
    bool Open(unsigned short port, std::string* error);
    void Start();

private:
    tcp::acceptor _acceptor;
};

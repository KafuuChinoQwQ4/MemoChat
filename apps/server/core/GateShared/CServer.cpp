#include "CServer.hpp"
#include <iostream>
#include "HttpConnection.hpp"
#include "AsioIOServicePool.hpp"

CServer::CServer(boost::asio::io_context& ioc)
    : _acceptor(ioc)
{
}

bool CServer::Open(unsigned short port, std::string* error)
{
    boost::system::error_code ec;
    _acceptor.open(tcp::v4(), ec);
    if (!ec)
    {
        _acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
    }
    if (!ec)
    {
        _acceptor.bind(tcp::endpoint(tcp::v4(), port), ec);
    }
    if (!ec)
    {
        _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    }
    if (!ec)
    {
        return true;
    }

    if (error != nullptr)
    {
        *error = ec.message();
    }
    boost::system::error_code close_ec;
    _acceptor.close(close_ec);
    return false;
}

void CServer::Start()
{
    auto self = weak_from_this().lock();
    if (!self || !_acceptor.is_open())
    {
        return;
    }

    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    auto new_con = std::make_shared<HttpConnection>(io_context);
    auto& socket = new_con->GetSocket();
    _acceptor.async_accept(socket,
                           [self = std::move(self), new_con = std::move(new_con)](const boost::system::error_code& ec)
                           {
                               if (!ec)
                               {
                                   boost::system::error_code option_ec;
                                   new_con->GetSocket().set_option(tcp::no_delay(true), option_ec);
                                   new_con->Start();
                               }
                               else if (ec != boost::asio::error::operation_aborted)
                               {
                                   std::cout << "accept failed, error is " << ec.message() << std::endl;
                               }

                               if (self->_acceptor.is_open())
                               {
                                   self->Start();
                               }
                           });
}

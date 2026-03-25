#pragma once

#include <string>
#include "const.h"

class H1Listener :public std::enable_shared_from_this<H1Listener>
{
public:
	H1Listener(boost::asio::io_context& ioc, unsigned short& port);
	void Start();
private:
	tcp::acceptor  _acceptor;
	net::io_context& _ioc;
};

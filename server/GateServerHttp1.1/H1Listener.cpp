#include "H1Listener.h"
#include "H1Connection.h"
#include "AsioIOServicePool.h"

H1Listener::H1Listener(boost::asio::io_context& ioc, unsigned short& port)
    : _ioc(ioc),
      _acceptor(ioc, tcp::endpoint(tcp::v4(), port)) {

}

void H1Listener::Start()
{
	auto self = shared_from_this();
	auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
	std::shared_ptr<H1Connection> new_con = std::make_shared<H1Connection>(io_context);
	_acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
		try {

			if (ec) {
				self->Start();
				return;
			}

			beast::error_code option_ec;
			new_con->GetSocket().set_option(tcp::no_delay(true), option_ec);

			new_con->Start();

			self->Start();
		}
		catch (std::exception& exp) {
			std::cout << "exception is " << exp.what() << std::endl;
			self->Start();
		}
	});
}

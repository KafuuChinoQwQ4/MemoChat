#include "CServer.h"
#include "HttpConnection.h"
#include "AsioIOServicePool.h" // 引入头文件

CServer::CServer(boost::asio::io_context& ioc, unsigned short port)
    : _ioc(ioc), _acceptor(ioc, tcp::endpoint(tcp::v4(), port)), _socket(ioc) {
}

void CServer::Start()
{
    auto self = shared_from_this();
    // 核心修改：Acceptor 用主线程 context，但新 socket 用线程池的 context
    auto& io_context = AsioIOServicePool::GetInstance()->GetIOService();
    
    // 注意：这里需要创建一个新socket绑定到io_context上，原来的 _socket 是绑定在主 _ioc 上的
    std::shared_ptr<HttpConnection> new_con = std::make_shared<HttpConnection>(io_context);

    _acceptor.async_accept(new_con->GetSocket(), [self, new_con](beast::error_code ec) {
        try {
            if (ec) {
                self->Start();
                return;
            }
            new_con->Start();
            self->Start();
        }
        catch (std::exception& exp) {
            std::cout << "exception is " << exp.what() << std::endl;
            self->Start();
        }
    });
}
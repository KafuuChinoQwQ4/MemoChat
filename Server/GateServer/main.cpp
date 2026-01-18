#include <iostream>
#include "CServer.h"
#include "ConfigMgr.h" // 1. 引入配置管理头文件

int main()
{
    try
    {
        // 2. 从配置中读取端口 (不再写死 8080)
        // gCfgMgr 是我们在 ConfigMgr.cpp 里定义的全局变量
        std::string port_str = gCfgMgr["GateServer"]["Port"];
        
        if (port_str.empty()) {
            port_str = "8080"; // 如果没读到，给个默认值防止崩
            std::cout << "Warning: Read config failed, use default port 8080" << std::endl;
        }

        unsigned short port = static_cast<unsigned short>(std::atoi(port_str.c_str()));

        net::io_context ioc{ 1 };
        
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait([&ioc](const boost::system::error_code& error, int signal_number) {
            if (error) {
                return;
            }
            ioc.stop();
        });

        std::make_shared<CServer>(ioc, port)->Start();
        std::cout << "GateServer listen on port: " << port << std::endl;
        ioc.run();
    }
    catch (std::exception const& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
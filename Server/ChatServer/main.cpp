#include <iostream>
#include <boost/asio.hpp>
#include "CServer.h"
#include "../GateServer/ConfigMgr.h" // 假设你直接引用 GateServer 的代码

int main()
{
    try {
        // 读取配置 (确保 config.ini 里有 [ChatServer] 及其 Port)
        auto& cfg = ConfigMgr::Inst();
        std::string port_str = cfg["ChatServer"]["Port"];
        
        boost::asio::io_context io_context;
        
        // 启动服务器
        CServer s(io_context, static_cast<unsigned short>(std::atoi(port_str.c_str())));
        
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
    return 0;
}
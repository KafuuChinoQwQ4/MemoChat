#include "CServer.h"
#include <iostream>
#include "AsioIOServicePool.h"
#include "UserMgr.h"
#include "ConfigMgr.h" // 必须包含
#include "RedisMgr.h"  // 必须包含
#include "const.h"

CServer::CServer(boost::asio::io_context& io_context, unsigned short port)
    : _io_context(io_context), _acceptor(io_context, tcp::endpoint(tcp::v4(), port))
{
    std::cout << "ChatServer listen on port: " << port << endl;
    AsyncAccept();
}

CServer::~CServer() {
    std::cout << "CServer destruct" << endl;
}

void CServer::AsyncAccept() {
    auto session = std::make_shared<CSession>(_io_context, this);
    _acceptor.async_accept(session->GetSocket(), [this, session](boost::system::error_code ec) {
        if (!ec) {
            std::cout << "New client connected: " << session->GetSessionId() << endl;
            session->Start();
            
            std::lock_guard<std::mutex> lock(_mutex);
            _sessions[session->GetSessionId()] = session;
        }
        AsyncAccept();
    });
}

void CServer::ClearSession(std::string session_id) {
    if (_sessions.find(session_id) != _sessions.end()) {
        // 1. 减少 Redis 计数
        auto server_name = ConfigMgr::Inst().GetValue("SelfServer", "Name");
        std::string count_str = RedisMgr::GetInstance()->HGet(LOGIN_COUNT, server_name);
        if (!count_str.empty()) {
             int count = std::stoi(count_str);
             if (count > 0) {
                 count--;
                 RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, std::to_string(count));
             }
        }
        
        // 2. 移除内存中的用户映射
        // 注意：_sessions[session_id] 是 shared_ptr，我们需要在 erase 之前获取 uid
        int uid = _sessions[session_id]->GetUserId();
        UserMgr::GetInstance()->RmvUserSession(uid);
    }

    {
        std::lock_guard<std::mutex> lock(_mutex);
        _sessions.erase(session_id);
    }
}
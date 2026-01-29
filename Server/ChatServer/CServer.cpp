#include "CServer.h"
#include <iostream>

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
    std::lock_guard<std::mutex> lock(_mutex);
    _sessions.erase(session_id);
}
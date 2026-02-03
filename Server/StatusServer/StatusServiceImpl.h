#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h"
#include <vector>
#include <string>
#include <mutex>
#include <map>

using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

// 定义 ChatServer 的结构信息
struct ChatServer {
    std::string name;
    std::string host;
    std::string port;
    int con_count = 0; // 连接数
};

class StatusServiceImpl final : public StatusService::Service
{
public:
    StatusServiceImpl();
    Status GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply) override;
    Status Login(ServerContext* context, const LoginReq* request, LoginRsp* reply) override;

private:
    ChatServer getChatServer(); // 获取最优服务器
    void insertToken(int uid, std::string token); // 存储 Token 到 Redis

    std::map<std::string, ChatServer> _servers; // 内存中存储的服务器列表
    std::mutex _server_mtx;
};
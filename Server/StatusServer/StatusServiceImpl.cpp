#include "StatusServiceImpl.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include <climits>
#include <sstream>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

// 生成 UUID 作为 Token
std::string generate_unique_string() {
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    return boost::uuids::to_string(uuid);
}

StatusServiceImpl::StatusServiceImpl()
{
    auto& cfg = ConfigMgr::Inst();
    auto server_list = cfg["chatservers"]["Name"];

    std::vector<std::string> words;
    std::stringstream ss(server_list);
    std::string word;
    // 解析配置文件中的服务器列表 (例如 "chatserver1,chatserver2")
    while (std::getline(ss, word, ',')) {
        words.push_back(word);
    }

    for (auto& name : words) {
        ChatServer server;
        server.name = name;
        server.host = cfg[name]["Host"];
        server.port = cfg[name]["Port"];
        _servers[name] = server;
    }
}

Status StatusServiceImpl::GetChatServer(ServerContext* context, const GetChatServerReq* request, GetChatServerRsp* reply)
{
    std::cout << "Received GetChatServer Req for uid: " << request->uid() << std::endl;
    
    // 1. 查找负载最小的服务器
    const auto& server = getChatServer();
    
    // 2. 生成 Token
    std::string token = generate_unique_string();
    
    // 3. 填充响应
    reply->set_host(server.host);
    reply->set_port(server.port);
    reply->set_error(0); // Success
    reply->set_token(token);
    
    // 4. 将 Token 存入 Redis 供 ChatServer 验证
    insertToken(request->uid(), token);
    
    return Status::OK;
}

Status StatusServiceImpl::Login(ServerContext* context, const LoginReq* request, LoginRsp* reply)
{
    // StatusServer 也可以处理登录，或者仅做 Token 分发，这里保留接口
    return Status::OK;
}

ChatServer StatusServiceImpl::getChatServer() {
    std::lock_guard<std::mutex> guard(_server_mtx);
    
    ChatServer minServer;
    int minCount = INT_MAX;
    
    for (auto& pair : _servers) {
        auto& server = pair.second;
        // 从 Redis 读取该服务器当前的连接数
        // 注意：ChatServer 必须在登录/登出时更新这个 key (LOGIN_COUNT)
        std::string count_str = RedisMgr::GetInstance()->HGet("login_count", server.name);
        
        if (count_str.empty()) {
            server.con_count = 0;
        } else {
            server.con_count = std::stoi(count_str);
        }

        // 挑选连接数最小的
        if (server.con_count < minCount) {
            minCount = server.con_count;
            minServer = server;
        }
    }
    
    std::cout << "Selected server: " << minServer.name << " with count: " << minCount << std::endl;
    return minServer;
}

void StatusServiceImpl::insertToken(int uid, std::string token) {
    // Key 格式: utoken_UID
    std::string uid_str = std::to_string(uid);
    std::string token_key = "utoken_" + uid_str;
    RedisMgr::GetInstance()->Set(token_key, token);
}
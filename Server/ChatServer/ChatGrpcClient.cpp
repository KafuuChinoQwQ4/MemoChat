#include "ChatGrpcClient.h"
#include "const.h"
#include <iostream>

ChatGrpcClient::ChatGrpcClient() {
    // 构造函数保持为空，在 Initialize 中初始化，或者直接在这里初始化也可以
    // 建议在 main 或系统启动时显式调用 Initialize 以便控制顺序
}

void ChatGrpcClient::Initialize() {
    auto& cfg = ConfigMgr::Inst();
    auto server_list = cfg["PeerServer"]["Servers"];

    std::vector<std::string> words;
    std::stringstream ss(server_list);
    std::string word;

    while (std::getline(ss, word, ',')) {
        words.push_back(word);
    }

    for (auto& word : words) {
        if (cfg[word]["Name"].empty()) {
            continue;
        }
        // 读取对端服务器的配置，创建连接池（池大小设为5）
        _pools[cfg[word]["Name"]] = std::make_unique<ChatConPool>(5, cfg[word]["Host"], cfg[word]["Port"]);
    }
}

AddFriendRsp ChatGrpcClient::NotifyAddFriend(std::string server_name, const AddFriendReq& req) {
    AddFriendRsp rsp;
    rsp.set_error(ErrorCodes::Error_Json);

    auto iter = _pools.find(server_name);
    if (iter == _pools.end()) {
        return rsp;
    }

    auto stub = iter->second->getConnection();
    if (!stub) {
        return rsp;
    }

    ClientContext context;
    Status status = stub->NotifyAddFriend(&context, req, &rsp);
    
    iter->second->returnConnection(std::move(stub));

    if (!status.ok()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    return rsp;
}

AuthFriendRsp ChatGrpcClient::NotifyAuthFriend(std::string server_name, const AuthFriendReq& req) {
    AuthFriendRsp rsp;
    rsp.set_error(ErrorCodes::Error_Json);

    auto iter = _pools.find(server_name);
    if (iter == _pools.end()) {
        return rsp;
    }

    auto stub = iter->second->getConnection();
    if (!stub) {
        return rsp;
    }

    ClientContext context;
    Status status = stub->NotifyAuthFriend(&context, req, &rsp);
    iter->second->returnConnection(std::move(stub));

    if (!status.ok()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    return rsp;
}

TextChatMsgRsp ChatGrpcClient::NotifyTextChatMsg(std::string server_name, const TextChatMsgReq& req, const Json::Value& rtvalue) {
    TextChatMsgRsp rsp;
    rsp.set_error(ErrorCodes::Error_Json);
    
    auto iter = _pools.find(server_name);
    if (iter == _pools.end()) {
        return rsp;
    }
    
    auto stub = iter->second->getConnection();
    if (!stub) {
        return rsp;
    }
    
    ClientContext context;
    Status status = stub->NotifyTextChatMsg(&context, req, &rsp);
    iter->second->returnConnection(std::move(stub));
    
    if (!status.ok()) {
         rsp.set_error(ErrorCodes::RPCFailed);
         return rsp;
    }
    
    return rsp;
}
#pragma once

#include "Singleton.hpp"
#include "ConfigMgr.hpp"
#include "const.hpp"
#include <grpcpp/grpcpp.h>
#include "common/proto/message.grpc.pb.h"
#include "common/proto/message.pb.h"
#include <condition_variable>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using message::ChatService;
using message::GroupEventNotifyReq;
using message::GroupEventNotifyRsp;

class ChatConPool
{
public:
    ChatConPool(size_t poolSize, std::string host, std::string port);

    std::unique_ptr<ChatService::Stub> getConnection();
    void returnConnection(std::unique_ptr<ChatService::Stub> context);
    void Close();

private:
    std::atomic<bool> b_stop_;
    std::queue<std::unique_ptr<ChatService::Stub>> connections_;
    std::mutex mutex_;
    std::condition_variable cond_;
};

class ChatGrpcClient : public Singleton<ChatGrpcClient>
{
    friend class Singleton<ChatGrpcClient>;

public:
    GroupEventNotifyRsp NotifyCallEvent(const std::string& server_name, const GroupEventNotifyReq& req);

private:
    ChatGrpcClient();
    std::unordered_map<std::string, std::unique_ptr<ChatConPool>> _pools;
};

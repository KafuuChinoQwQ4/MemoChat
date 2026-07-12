#include "ChatGrpcClient.hpp"

#include "logging/GrpcTrace.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "cluster/ChatClusterDiscovery.hpp"

#include <utility>

import memochat.gate.chat_grpc_client_algorithms;

namespace chat_grpc_modules = memochat::gate::chat_grpc_client::modules;

ChatConPool::ChatConPool(size_t poolSize, std::string host, std::string port)
    : b_stop_(false)
{
    for (size_t i = 0; i < poolSize; ++i)
    {
        std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
        connections_.push(ChatService::NewStub(channel));
    }
}

std::unique_ptr<ChatService::Stub> ChatConPool::getConnection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock,
               [this]
               {
                   return chat_grpc_modules::ShouldWakeConnectionWait(b_stop_, !connections_.empty());
               });
    if (chat_grpc_modules::ShouldReturnNullConnection(b_stop_))
    {
        return nullptr;
    }
    auto stub = std::move(connections_.front());
    connections_.pop();
    return stub;
}

void ChatConPool::returnConnection(std::unique_ptr<ChatService::Stub> context)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!chat_grpc_modules::ShouldAcceptReturnedConnection(b_stop_))
    {
        return;
    }
    connections_.push(std::move(context));
    cond_.notify_one();
}

void ChatConPool::Close()
{
    b_stop_ = true;
    cond_.notify_all();
}

ChatGrpcClient::ChatGrpcClient()
{
    auto& cfg = ConfigMgr::Inst();
    memochat::cluster::ChatClusterConfig cluster;
    std::string cluster_error;
    if (!memochat::cluster::LoadChatClusterConfig(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            },
            std::string(),
            cluster,
            cluster_error))
    {
        memolog::LogError("gate.chat_grpc.cluster_config_invalid",
                          "ChatGrpcClient cluster config is invalid",
                          {{"error", cluster_error}});
        return;
    }
    for (const auto& node : cluster.enabledNodes())
    {
        _pools[node.name] =
            std::make_unique<ChatConPool>(chat_grpc_modules::DefaultConnectionPoolSize(), node.rpc_host, node.rpc_port);
    }
}

GroupEventNotifyRsp ChatGrpcClient::NotifyCallEvent(const std::string& server_name, const GroupEventNotifyReq& req)
{
    memolog::SpanScope span(chat_grpc_modules::NotifyCallEventSpanName(),
                            chat_grpc_modules::ClientSpanKind(),
                            {{chat_grpc_modules::RpcSystemAttribute(), chat_grpc_modules::RpcSystemValue()},
                             {chat_grpc_modules::RpcServiceAttribute(), chat_grpc_modules::ChatServiceName()},
                             {chat_grpc_modules::RpcMethodAttribute(), chat_grpc_modules::NotifyGroupEventMethod()},
                             {chat_grpc_modules::PeerServiceAttribute(), server_name}});
    GroupEventNotifyRsp rsp;
    rsp.set_error(ErrorCodes::Success);

    const auto it = _pools.find(server_name);
    if (chat_grpc_modules::ShouldReportMissingServer(it != _pools.end()))
    {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    auto stub = it->second->getConnection();
    if (chat_grpc_modules::ShouldReportUnavailableStub(stub != nullptr))
    {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }
    Status status = stub->NotifyGroupEvent(&context, req, &rsp);
    it->second->returnConnection(std::move(stub));
    if (chat_grpc_modules::ShouldReportFailedStatus(status.ok()))
    {
        span.SetStatusError(chat_grpc_modules::RpcSystemValue(), status.error_message());
        rsp.set_error(ErrorCodes::RPCFailed);
    }
    return rsp;
}

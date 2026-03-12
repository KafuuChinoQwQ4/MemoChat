#include "ChatGrpcClient.h"

#include "logging/GrpcTrace.h"
#include "logging/Telemetry.h"
#include "cluster/ChatClusterDiscovery.h"

std::unique_ptr<ChatService::Stub> ChatConPool::getConnection()
{
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] { return b_stop_ || !connections_.empty(); });
    if (b_stop_) {
        return nullptr;
    }
    auto stub = std::move(connections_.front());
    connections_.pop();
    return stub;
}

void ChatConPool::returnConnection(std::unique_ptr<ChatService::Stub> context)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop_) {
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
    const auto cluster = memochat::cluster::LoadStaticChatClusterConfig(
        [&cfg](const std::string& section, const std::string& key) {
            return cfg.GetValue(section, key);
        });
    for (const auto& node : cluster.enabledNodes()) {
        _pools[node.name] = std::make_unique<ChatConPool>(5, node.rpc_host, node.rpc_port);
    }
}

GroupEventNotifyRsp ChatGrpcClient::NotifyCallEvent(const std::string& server_name, const GroupEventNotifyReq& req)
{
    memolog::SpanScope span("ChatService.NotifyCallEvent", "CLIENT",
        {{"rpc.system", "grpc"}, {"rpc.service", "ChatService"}, {"rpc.method", "NotifyGroupEvent"}, {"peer_service", server_name}});
    GroupEventNotifyRsp rsp;
    rsp.set_error(ErrorCodes::Success);

    const auto it = _pools.find(server_name);
    if (it == _pools.end()) {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }

    ClientContext context;
    memolog::InjectGrpcTraceMetadata(context);
    auto stub = it->second->getConnection();
    if (!stub) {
        rsp.set_error(ErrorCodes::RPCFailed);
        return rsp;
    }
    Status status = stub->NotifyGroupEvent(&context, req, &rsp);
    it->second->returnConnection(std::move(stub));
    if (!status.ok()) {
        span.SetStatusError("grpc", status.error_message());
        rsp.set_error(ErrorCodes::RPCFailed);
    }
    return rsp;
}

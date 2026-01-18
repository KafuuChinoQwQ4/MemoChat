#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

// --- 连接池实现 ---
RPConPool::RPConPool(size_t poolSize, std::string host, std::string port)
    : poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
    for (size_t i = 0; i < poolSize_; ++i) {
        std::shared_ptr<Channel> channel = grpc::CreateChannel(host + ":" + port,
            grpc::InsecureChannelCredentials());
        connections_.push(VarifyService::NewStub(channel));
    }
}

RPConPool::~RPConPool() {
    std::lock_guard<std::mutex> lock(mutex_);
    Close();
    while (!connections_.empty()) {
        connections_.pop();
    }
}

std::unique_ptr<VarifyService::Stub> RPConPool::getConnection() {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [this] {
        if (b_stop_) return true;
        return !connections_.empty();
    });
    if (b_stop_) return nullptr;
    auto context = std::move(connections_.front());
    connections_.pop();
    return context;
}

void RPConPool::returnConnection(std::unique_ptr<VarifyService::Stub> context) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (b_stop_) return;
    connections_.push(std::move(context));
    cond_.notify_one();
}

void RPConPool::Close() {
    b_stop_ = true;
    cond_.notify_all();
}

// --- Client 修改 ---
VerifyGrpcClient::VerifyGrpcClient() {
    //auto& gCfgMgr = ConfigMgr::Inst(); // 注意这里要改一下 ConfigMgr
    std::string host = ConfigMgr::Inst()["VarifyServer"]["Host"];
    std::string port = ConfigMgr::Inst()["VarifyServer"]["Port"];
    pool_.reset(new RPConPool(5, host, port));
}

GetVarifyRsp VerifyGrpcClient::GetVarifyCode(std::string email) {
    ClientContext context;
    GetVarifyRsp reply;
    GetVarifyReq request;
    request.set_email(email);

    auto stub = pool_->getConnection(); // 获取连接
    Status status = stub->GetVarifyCode(&context, request, &reply);

    pool_->returnConnection(std::move(stub)); // 归还连接

    if (status.ok()) {
        return reply;
    } else {
        reply.set_error(ErrorCodes::RPCFailed);
        return reply;
    }
}
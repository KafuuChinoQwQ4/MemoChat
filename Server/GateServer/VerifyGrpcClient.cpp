#include "VerifyGrpcClient.h"
#include "ConfigMgr.h"

VerifyGrpcClient::VerifyGrpcClient() {
    // 后面我们会把端口配置在 config.ini 里
    // 为了防止出错，这里也可以先写死，或者从 ConfigMgr 读取
    std::string port = gCfgMgr["VarifyServer"]["Port"];
    if (port.empty()) port = "50051"; // 默认端口

    std::string address = "127.0.0.1:" + port;
    auto channel = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
    stub_ = VarifyService::NewStub(channel);
}

GetVarifyRsp VerifyGrpcClient::GetVarifyCode(std::string email) {
    ClientContext context;
    GetVarifyRsp reply;
    GetVarifyReq request;
    request.set_email(email);

    Status status = stub_->GetVarifyCode(&context, request, &reply);

    if (status.ok()) {
        return reply;
    }
    else {
        reply.set_error(ErrorCodes::RPCFailed);
        return reply;
    }
}
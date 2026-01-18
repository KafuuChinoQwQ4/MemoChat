#pragma once
#include <grpcpp/grpcpp.h>
#include "message.grpc.pb.h" // CMake 会自动生成这个文件
#include "const.h"
#include "Singleton.h"
#include <memory>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class VerifyGrpcClient : public Singleton<VerifyGrpcClient>
{
    friend class Singleton<VerifyGrpcClient>;
public:
    GetVarifyRsp GetVarifyCode(std::string email);

private:
    VerifyGrpcClient();
    std::unique_ptr<VarifyService::Stub> stub_;
};
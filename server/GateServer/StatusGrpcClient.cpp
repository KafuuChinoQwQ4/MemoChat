#include "StatusGrpcClient.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
	ClientContext context;
	GetChatServerRsp reply;
	GetChatServerReq request;
	request.set_uid(uid);
	const std::string trace_id = memolog::TraceContext::EnsureTraceId();
	context.AddMetadata("x-trace-id", trace_id);
	memolog::LogInfo("grpc.call", "calling StatusService.GetChatServer",
		{ {"rpc", "StatusService.GetChatServer"}, {"uid", std::to_string(uid)} });
	auto stub = pool_->getConnection();
	Status status = stub->GetChatServer(&context, request, &reply);
	Defer defer([&stub, this]() {
		pool_->returnConnection(std::move(stub));
		});
	if (status.ok()) {
		return reply;
	}
	else {
		memolog::LogWarn("grpc.call.failed", "StatusService.GetChatServer failed",
			{ {"rpc", "StatusService.GetChatServer"}, {"error", status.error_message()} });
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

LoginRsp StatusGrpcClient::Login(int uid, std::string token)
{
	ClientContext context;
	LoginRsp reply;
	LoginReq request;
	request.set_uid(uid);
	request.set_token(token);
	const std::string trace_id = memolog::TraceContext::EnsureTraceId();
	context.AddMetadata("x-trace-id", trace_id);

	auto stub = pool_->getConnection();
	Status status = stub->Login(&context, request, &reply);
	Defer defer([&stub, this]() {
		pool_->returnConnection(std::move(stub));
		});
	if (status.ok()) {
		return reply;
	}
	else {
		memolog::LogWarn("grpc.call.failed", "StatusService.Login failed",
			{ {"rpc", "StatusService.Login"}, {"error", status.error_message()} });
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}


StatusGrpcClient::StatusGrpcClient()
{
	auto& gCfgMgr = ConfigMgr::Inst();
	std::string host = gCfgMgr["StatusServer"]["Host"];
	std::string port = gCfgMgr["StatusServer"]["Port"];
	pool_.reset(new StatusConPool(5, host, port));
}

#include "StatusGrpcClient.h"
#include "logging/GrpcTrace.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TraceContext.h"

GetChatServerRsp StatusGrpcClient::GetChatServer(int uid)
{
	ClientContext context;
	memolog::SpanScope span("StatusService.GetChatServer", "CLIENT",
		{{"rpc.system", "grpc"}, {"rpc.service", "StatusService"}, {"rpc.method", "GetChatServer"}});
	GetChatServerRsp reply;
	GetChatServerReq request;
	request.set_uid(uid);
	memolog::InjectGrpcTraceMetadata(context);
	memolog::LogInfo("grpc.call", "calling StatusService.GetChatServer",
		{ {"rpc", "StatusService.GetChatServer"}, {"uid", std::to_string(uid)}, {"peer_service", "StatusServer"}, {"module", "grpc"} });
	auto stub = pool_->getConnection();
	Status status = stub->GetChatServer(&context, request, &reply);
	Defer defer([&stub, this]() {
		pool_->returnConnection(std::move(stub));
		});
	if (status.ok()) {
		span.SetAttribute("rpc.grpc.status_code", std::to_string(status.error_code()));
		return reply;
	}
	else {
		span.SetStatusError("grpc", status.error_message());
		span.SetAttribute("rpc.grpc.status_code", std::to_string(status.error_code()));
		memolog::LogWarn("grpc.call.failed", "StatusService.GetChatServer failed",
			{ {"rpc", "StatusService.GetChatServer"}, {"error", status.error_message()}, {"peer_service", "StatusServer"}, {"module", "grpc"}, {"error_type", "grpc"} });
		reply.set_error(ErrorCodes::RPCFailed);
		return reply;
	}
}

LoginRsp StatusGrpcClient::Login(int uid, std::string token)
{
	ClientContext context;
	memolog::SpanScope span("StatusService.Login", "CLIENT",
		{{"rpc.system", "grpc"}, {"rpc.service", "StatusService"}, {"rpc.method", "Login"}});
	LoginRsp reply;
	LoginReq request;
	request.set_uid(uid);
	request.set_token(token);
	memolog::InjectGrpcTraceMetadata(context);

	auto stub = pool_->getConnection();
	Status status = stub->Login(&context, request, &reply);
	Defer defer([&stub, this]() {
		pool_->returnConnection(std::move(stub));
		});
	if (status.ok()) {
		span.SetAttribute("rpc.grpc.status_code", std::to_string(status.error_code()));
		return reply;
	}
	else {
		span.SetStatusError("grpc", status.error_message());
		span.SetAttribute("rpc.grpc.status_code", std::to_string(status.error_code()));
		memolog::LogWarn("grpc.call.failed", "StatusService.Login failed",
			{ {"rpc", "StatusService.Login"}, {"error", status.error_message()}, {"peer_service", "StatusServer"}, {"module", "grpc"}, {"error_type", "grpc"} });
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

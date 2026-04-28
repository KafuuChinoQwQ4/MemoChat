#pragma once
#include <string>
#include <iostream>
#include <memory>
#include <grpcpp/grpcpp.h>
#include "common/proto/message.grpc.pb.h"
#include "const.h"
#include "Singleton.h"
#include "ConfigMgr.h"
#include "logging/GrpcTrace.h"
#include "logging/Telemetry.h"
using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;

using message::GetVarifyReq;
using message::GetVarifyRsp;
using message::VarifyService;

class RPConPool {
public:
	RPConPool(size_t poolSize, std::string host, std::string port)
		: poolSize_(poolSize), host_(host), port_(port), b_stop_(false) {
		for (size_t i = 0; i < poolSize_; ++i) {
			
			std::shared_ptr<Channel> channel = grpc::CreateChannel(host+":"+port,
				grpc::InsecureChannelCredentials());

			connections_.push(VarifyService::NewStub(channel));
		}
	}

	~RPConPool() {
		std::lock_guard<std::mutex> lock(mutex_);
		Close();
		while (!connections_.empty()) {
			connections_.pop();
		}
	}

	std::unique_ptr<VarifyService::Stub> getConnection() {
		std::unique_lock<std::mutex> lock(mutex_);
		cond_.wait(lock, [this] {
			if (b_stop_) {
				return true;
			}
			return !connections_.empty();
			});

		if (b_stop_) {
			return  nullptr;
		}
		auto context = std::move(connections_.front());
		connections_.pop();
		return context;
	}

	void returnConnection(std::unique_ptr<VarifyService::Stub> context) {
		std::lock_guard<std::mutex> lock(mutex_);
		if (b_stop_) {
			return;
		}
		connections_.push(std::move(context));
		cond_.notify_one();
	}

	void Close() {
		b_stop_ = true;
		cond_.notify_all();
	}

private:
	atomic<bool> b_stop_;
	size_t poolSize_;
	std::string host_;
	std::string port_;
	std::queue<std::unique_ptr<VarifyService::Stub>> connections_;
	std::mutex mutex_;
	std::condition_variable cond_;
};

class VerifyGrpcClient:public Singleton<VerifyGrpcClient>
{
	friend class Singleton<VerifyGrpcClient>;
public:
	~VerifyGrpcClient() {
		
	}
	GetVarifyRsp GetVarifyCode(std::string email) {
		ClientContext context;
		memolog::SpanScope span("VarifyService.GetVarifyCode", "CLIENT",
			{{"rpc.system", "grpc"}, {"rpc.service", "VarifyService"}, {"rpc.method", "GetVarifyCode"}});
		GetVarifyRsp reply;
		GetVarifyReq request;
		request.set_email(email);
		memolog::InjectGrpcTraceMetadata(context);
		auto stub = pool_->getConnection();
		Status status = stub->GetVarifyCode(&context, request, &reply);

		if (status.ok()) {
			span.SetAttribute("rpc.grpc.status_code", std::to_string(status.error_code()));
			pool_->returnConnection(std::move(stub));
			return reply;
		}
		else {
			span.SetStatusError("grpc", status.error_message());
			span.SetAttribute("rpc.grpc.status_code", std::to_string(status.error_code()));
			pool_->returnConnection(std::move(stub));
			reply.set_error(ErrorCodes::RPCFailed);
			return reply;
		}
	}

private:
	VerifyGrpcClient();

	std::unique_ptr<RPConPool> pool_;
};



#pragma once
#include <grpcpp/grpcpp.h>
#include "common/proto/message.grpc.pb.h"
#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

class StatusAsyncSideEffects;

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using message::GetChatServerReq;
using message::GetChatServerRsp;
using message::LoginReq;
using message::LoginRsp;
using message::StatusService;

class  ChatServer {
public:
	ChatServer():host(""),port(""),name(""),con_count(0){}
	ChatServer(const ChatServer& cs):host(cs.host), port(cs.port), name(cs.name), con_count(cs.con_count){}
	ChatServer& operator=(const ChatServer& cs) {
		if (&cs == this) {
			return *this;
		}

		host = cs.host;
		name = cs.name;
		port = cs.port;
		con_count = cs.con_count;
		return *this;
	}
	std::string host;
	std::string port;
	std::string name;
	int con_count;
};
class StatusServiceImpl final : public StatusService::Service
{
public:
	StatusServiceImpl();
	~StatusServiceImpl() override;
	Status GetChatServer(ServerContext* context, const GetChatServerReq* request,
		GetChatServerRsp* reply) override;
	Status Login(ServerContext* context, const LoginReq* request,
		LoginRsp* reply) override;
private:
	void insertToken(int uid, std::string token);
	ChatServer getChatServer(std::vector<std::string>* server_load_snapshot = nullptr,
	                        std::vector<std::string>* least_loaded_servers_snapshot = nullptr);
	std::unordered_map<std::string, ChatServer> _servers;
	std::mutex _server_mtx;
	std::atomic<uint64_t> _rr_counter{0};
    std::unordered_set<std::string> _known_server_names;
    std::unique_ptr<StatusAsyncSideEffects> _side_effects;

};

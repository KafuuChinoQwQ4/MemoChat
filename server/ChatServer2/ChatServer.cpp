
//

#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
#include <unordered_set>
#include <vector>
#include "AsioIOServicePool.h"
#include "CServer.h"
#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "ChatServiceImpl.h"
#include "const.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"

using namespace std;
bool bstop = false;
std::condition_variable cond_quit;
std::mutex mutex_quit;

namespace {
std::string ServerOnlineUsersKey(const std::string& server_name) {
	return std::string(SERVER_ONLINE_USERS_PREFIX) + server_name;
}

void CleanupTrackedOnlineState(const std::string& server_name) {
	std::unordered_set<std::string> online_uids;
	const auto online_key = ServerOnlineUsersKey(server_name);
	std::vector<std::string> tracked_uids;
	RedisMgr::GetInstance()->SMembers(online_key, tracked_uids);
	for (const auto& uid : tracked_uids) {
		if (!uid.empty()) {
			online_uids.insert(uid);
		}
	}

	std::vector<std::string> ip_keys;
	RedisMgr::GetInstance()->Keys(std::string(USERIPPREFIX) + "*", ip_keys);
	for (const auto& ip_key : ip_keys) {
		if (ip_key.rfind(USERIPPREFIX, 0) != 0) {
			continue;
		}
		std::string mapped_server;
		if (!RedisMgr::GetInstance()->Get(ip_key, mapped_server) || mapped_server != server_name) {
			continue;
		}
		online_uids.insert(ip_key.substr(std::strlen(USERIPPREFIX)));
	}

	for (const auto& uid : online_uids) {
		if (uid.empty()) {
			continue;
		}
		RedisMgr::GetInstance()->Del(USERIPPREFIX + uid);
		RedisMgr::GetInstance()->Del(USER_SESSION_PREFIX + uid);
	}
	RedisMgr::GetInstance()->Del(online_key);
	RedisMgr::GetInstance()->InitCount(server_name);
	memolog::LogInfo("service.online_state_reset",
		"ChatServer2 online state reset",
		{
			{"name", server_name},
			{"tracked_uid_count", std::to_string(online_uids.size())}
		});
}
}

int main()
{
	auto& cfg = ConfigMgr::Inst();
	auto server_name = cfg["SelfServer"]["Name"];
	try {
		auto log_cfg = memolog::LogConfig::FromGetter(
			[&cfg](const std::string& section, const std::string& key) {
				return cfg.GetValue(section, key);
			});
		memolog::Logger::Init("ChatServer2", log_cfg);
		auto pool = AsioIOServicePool::GetInstance();

		CleanupTrackedOnlineState(server_name);
		Defer derfer ([server_name]() {
				CleanupTrackedOnlineState(server_name);
				RedisMgr::GetInstance()->DelCount(server_name);
				RedisMgr::GetInstance()->Close();
				memolog::Logger::Shutdown();
			});

		boost::asio::io_context  io_context;
		auto port_str = cfg["SelfServer"]["Port"];

		auto pointer_server = std::make_shared<CServer>(io_context, atoi(port_str.c_str()));

		pointer_server->StartTimer();



		std::string server_address(cfg["SelfServer"]["Host"] + ":" + cfg["SelfServer"]["RPCPort"]);
		ChatServiceImpl service;
		grpc::ServerBuilder builder;

		builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
		builder.RegisterService(&service);
		service.RegisterServer(pointer_server);

		std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
		memolog::LogInfo("service.start", "ChatServer2 listening", { {"rpc_address", server_address}, {"name", server_name} });


		std::thread  grpc_server_thread([&server]() {
				server->Wait();
			});

	
		boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
		signals.async_wait([&io_context, pool, &server](auto, auto) {
			io_context.stop();
			pool->Stop();
			server->Shutdown();
			});
		
	

		LogicSystem::GetInstance()->SetServer(pointer_server);
		io_context.run();

		grpc_server_thread.join();
		pointer_server->StopTimer();
		memolog::LogInfo("service.stop", "ChatServer2 stopped");
		return 0;
	}
	catch (std::exception& e) {
		memolog::LogError("service.fatal", "ChatServer2 crashed", { {"error", e.what()} });
		memolog::Logger::Shutdown();
		return EXIT_FAILURE;
	}

}

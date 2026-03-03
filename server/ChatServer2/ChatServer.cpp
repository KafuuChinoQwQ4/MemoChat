
//

#include "LogicSystem.h"
#include <csignal>
#include <thread>
#include <mutex>
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

		RedisMgr::GetInstance()->HSet(LOGIN_COUNT, server_name, "0");
		Defer derfer ([server_name]() {
				RedisMgr::GetInstance()->HDel(LOGIN_COUNT, server_name);
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


//

#include <iostream>
#include "const.h"
#include "ConfigMgr.h"
#include <hiredis/hiredis.h>
#include "RedisMgr.h"
#include "PostgresMgr.h"
#include "AsioIOServicePool.h"
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "StatusServiceImpl.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"
void RunServer() {
	auto & cfg = ConfigMgr::Inst();
	
	std::string server_address(cfg["StatusServer"]["Host"]+":"+ cfg["StatusServer"]["Port"]);
	StatusServiceImpl service;

	grpc::ServerBuilder builder;

	builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
	builder.RegisterService(&service);


	std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
	memolog::LogInfo("service.start", "StatusServer listening", { {"address", server_address} });


	boost::asio::io_context io_context;

	boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);


	signals.async_wait([&server, &io_context](const boost::system::error_code& error, int signal_number) {
		if (!error) {
			memolog::LogInfo("service.stop", "StatusServer shutting down");
			server->Shutdown();
			io_context.stop();
		}
		});


	std::thread([&io_context]() { io_context.run(); }).detach();


	server->Wait();

}

int main(int argc, char** argv) {
	try {
		auto& cfg = ConfigMgr::Inst();
		auto log_cfg = memolog::LogConfig::FromGetter(
			[&cfg](const std::string& section, const std::string& key) {
				return cfg.GetValue(section, key);
			});
		auto telemetry_cfg = memolog::TelemetryConfig::FromGetter(
			[&cfg](const std::string& section, const std::string& key) {
				return cfg.GetValue(section, key);
			});
		memolog::Logger::Init("StatusServer", log_cfg);
		memolog::Telemetry::Init("StatusServer", telemetry_cfg);
		RunServer();
		RedisMgr::GetInstance()->Close();
		memolog::Telemetry::Shutdown();
		memolog::Logger::Shutdown();
	}
	catch (std::exception const& e) {
		memolog::LogError("service.fatal", "StatusServer crashed", { {"error", e.what()} });
		RedisMgr::GetInstance()->Close();
		memolog::Telemetry::Shutdown();
		memolog::Logger::Shutdown();
		return EXIT_FAILURE;
	}

	return 0;
}


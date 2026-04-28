#include <iostream>
#include <memory>
#include <thread>
#include <boost/asio.hpp>
#include <grpcpp/grpcpp.h>
#include "AIServiceImpl.h"
#include "ConfigMgr.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/TelemetryConfig.h"
#include "logging/Telemetry.h"

void RunServer() {
    auto& cfg = ConfigMgr::Inst();

    std::string server_address(cfg["AIServer"]["Host"] + ":" + cfg["AIServer"]["Port"]);
    AIServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    memolog::LogInfo("service.start", "AIServer listening", {{"address", server_address}});

    boost::asio::io_context io_context;
    boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&server, &io_context](const boost::system::error_code& error, int signal_number) {
        if (!error) {
            memolog::LogInfo("service.stop", "AIServer shutting down");
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
        memolog::Logger::Init("AIServer", log_cfg);
        memolog::Telemetry::Init("AIServer", telemetry_cfg);

        memolog::LogInfo("app.start", "AIServer starting up");

        RunServer();

        memolog::LogInfo("app.exit", "AIServer exited");
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
    }
    catch (const std::exception& e) {
        memolog::LogError("service.fatal", "AIServer crashed", {{"error", e.what()}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
    return 0;
}

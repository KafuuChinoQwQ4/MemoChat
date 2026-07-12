#include <iostream>
#include <memory>
#include <boost/asio.hpp>
#include <grpcpp/grpcpp.h>
#include "AIServiceImpl.hpp"
#include "ConfigMgr.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/TelemetryConfig.hpp"
#include "logging/Telemetry.hpp"
#include "runtime/ExplicitThread.hpp"

bool RunServer()
{
    auto& cfg = ConfigMgr::Inst();

    std::string server_address(cfg["AIServer"]["Host"] + ":" + cfg["AIServer"]["Port"]);
    AIServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server)
    {
        memolog::LogError("service.start_failed", "AIServer failed to listen", {{"address", server_address}});
        return false;
    }
    memolog::LogInfo("service.start", "AIServer listening", {{"address", server_address}});

    boost::asio::io_context io_context;
    boost::asio::signal_set signals(io_context);
    boost::system::error_code signal_error;
    signals.add(SIGINT, signal_error);
    if (!signal_error)
    {
        signals.add(SIGTERM, signal_error);
    }
    if (signal_error)
    {
        memolog::LogError("service.signal_setup_failed",
                          "AIServer failed to register shutdown signals",
                          {{"error", signal_error.message()}});
        server->Shutdown();
        return false;
    }
    signals.async_wait(
        [&server, &io_context](const boost::system::error_code& error, int signal_number)
        {
            if (!error)
            {
                memolog::LogInfo("service.stop", "AIServer shutting down");
                server->Shutdown();
                io_context.stop();
            }
        });

    memochat::runtime::ExplicitThread signal_thread;
    std::string thread_error;
    if (!signal_thread.Start(
            [&io_context]() noexcept
            {
                io_context.run();
            },
            &thread_error))
    {
        memolog::LogError("service.thread_start_failed",
                          "AIServer failed to start signal thread",
                          {{"error", thread_error}});
        server->Shutdown();
        return false;
    }
    server->Wait();
    io_context.stop();
    if (!signal_thread.Join(&thread_error))
    {
        memolog::LogError("service.thread_join_failed",
                          "AIServer failed to join signal thread",
                          {{"error", thread_error}});
        return false;
    }
    return true;
}

int main(int argc, char** argv)
{
    (void) argc;
    (void) argv;
    auto& cfg = ConfigMgr::Inst();
    auto log_cfg = memolog::LogConfig::FromGetter(
        [&cfg](const std::string& section, const std::string& key)
        {
            return cfg.GetValue(section, key);
        });
    auto telemetry_cfg = memolog::TelemetryConfig::FromGetter(
        [&cfg](const std::string& section, const std::string& key)
        {
            return cfg.GetValue(section, key);
        });
    std::string logger_error;
    if (!memolog::Logger::Init("AIServer", log_cfg, &logger_error))
    {
        std::cerr << "AIServer fatal: " << logger_error << std::endl;
        return EXIT_FAILURE;
    }
    memolog::Telemetry::Init("AIServer", telemetry_cfg);

    memolog::LogInfo("app.start", "AIServer starting up");
    const bool started = RunServer();
    memolog::LogInfo("app.exit", "AIServer exited");
    memolog::Telemetry::Shutdown();
    memolog::Logger::Shutdown();
    return started ? EXIT_SUCCESS : EXIT_FAILURE;
}

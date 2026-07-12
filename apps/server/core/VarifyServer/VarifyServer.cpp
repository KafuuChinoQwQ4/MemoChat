#include "VarifyServer.hpp"

#include "VarifyServiceImpl.hpp"
#include "VarifyRedisMgr.hpp"
#include "EmailTaskBus.hpp"
#include "ConfigMgr.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"
#include "runtime/ExplicitThread.hpp"

#include <charconv>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <grpcpp/grpcpp.h>

import memochat.varify.server_runtime_algorithms;

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = net::ip::tcp;

namespace
{
namespace varify_runtime_modules = memochat::varify::server::modules;

void RunHealthServer(int port)
{
    net::io_context ioc{1};
    tcp::acceptor acceptor(ioc);
    beast::error_code ec;
    acceptor.open(tcp::v4(), ec);
    if (!ec)
    {
        acceptor.set_option(tcp::acceptor::reuse_address(true), ec);
    }
    if (!ec)
    {
        acceptor.bind(tcp::endpoint(tcp::v4(), static_cast<unsigned short>(port)), ec);
    }
    if (!ec)
    {
        acceptor.listen(net::socket_base::max_listen_connections, ec);
    }
    if (ec)
    {
        memolog::LogError("service.health.error", "health listener failed", {{"error", ec.message()}});
        return;
    }

    memolog::LogInfo("service.health.start",
                     "VarifyServer health endpoint started",
                     {{"address", std::string("0.0.0.0:") + std::to_string(port)}});

    for (;;)
    {
        tcp::socket socket(ioc);
        acceptor.accept(socket, ec);
        if (ec)
        {
            memolog::LogWarn("service.health.accept_failed", "health accept failed", {{"error", ec.message()}});
            continue;
        }

        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        http::read(socket, buffer, req, ec);
        if (ec)
        {
            beast::error_code close_ec;
            socket.close(close_ec);
            continue;
        }

        std::string target = std::string(req.target());
        const bool is_health = varify_runtime_modules::IsHealthPath(target.data(), target.size());
        const bool is_ready = varify_runtime_modules::IsReadinessPath(target.data(), target.size());

        if (varify_runtime_modules::ShouldReplyNotFound(is_health, is_ready))
        {
            http::response<http::string_body> res{boost::beast::http::status::not_found, req.version()};
            res.set(http::field::content_type, varify_runtime_modules::TextContentType());
            res.body() = varify_runtime_modules::NotFoundBody();
            res.prepare_payload();
            http::write(socket, res, ec);
            beast::error_code close_ec;
            socket.close(close_ec);
            continue;
        }

        bool redis_ok = varifyservice::VarifyRedisMgr::Instance().Ping();

        http::response<http::string_body> res{boost::beast::http::status::ok, req.version()};
        res.set(http::field::content_type, varify_runtime_modules::JsonContentType());

        if (varify_runtime_modules::ShouldReportRedisDown(is_ready, redis_ok))
        {
            res.result(http::status::service_unavailable);
            res.body() = varify_runtime_modules::RedisDownBody();
        }
        else if (is_ready)
        {
            res.body() = varify_runtime_modules::ReadyBody();
        }
        else
        {
            res.body() = varify_runtime_modules::HealthBody();
        }

        res.prepare_payload();
        http::write(socket, res, ec);
        beast::error_code close_ec;
        socket.close(close_ec);
    }
}

bool RunGrpcServer(const std::string& address)
{
    varifyservice::VarifyServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server)
    {
        memolog::LogError("service.start_failed", "failed to start VarifyServer gRPC server", {{"address", address}});
        return false;
    }
    memolog::LogInfo("service.start", "VarifyServer listening", {{"address", address}, {"module", "grpc"}});

    net::io_context io_context;
    net::signal_set signals(io_context);
    boost::system::error_code signal_error;
    signals.add(SIGINT, signal_error);
    if (!signal_error)
    {
        signals.add(SIGTERM, signal_error);
    }
    if (signal_error)
    {
        memolog::LogError("service.signal_init_failed",
                          "failed to install shutdown signals",
                          {{"error", signal_error.message()}});
        server->Shutdown();
        return false;
    }

    signals.async_wait(
        [&server, &io_context](const boost::system::error_code& error, int signal_number)
        {
            if (!error)
            {
                memolog::LogInfo("service.stop", "VarifyServer shutting down");
                server->Shutdown();
                io_context.stop();
            }
        });

    memochat::runtime::ExplicitThread signal_thread;
    std::string thread_error;
    if (!signal_thread.Start(
            [&io_context]()
            {
                io_context.run();
            },
            &thread_error))
    {
        memolog::LogError("service.signal_thread_start_failed",
                          "failed to start shutdown signal thread",
                          {{"error", thread_error}});
        server->Shutdown();
        return false;
    }

    server->Wait();
    io_context.stop();
    if (!signal_thread.Join(&thread_error))
    {
        memolog::LogError("service.signal_thread_join_failed",
                          "failed to join shutdown signal thread",
                          {{"error", thread_error}});
        return false;
    }
    return true;
}

} // anonymous namespace

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
    if (!memolog::Logger::Init("VarifyServer", log_cfg, &logger_error))
    {
        std::cerr << "VarifyServer fatal: " << logger_error << std::endl;
        return EXIT_FAILURE;
    }
    memolog::Telemetry::Init("VarifyServer", telemetry_cfg);

    std::string host = cfg["VarifyServer"]["Host"];
    std::string port = cfg["VarifyServer"]["Port"];
    std::string health_port_str = cfg["VarifyServer"]["HealthPort"];

    int health_port = varify_runtime_modules::DefaultHealthPort();
    if (varify_runtime_modules::ShouldUseConfiguredHealthPort(health_port_str.empty()))
    {
        int configured_port = 0;
        const auto [ptr, ec] =
            std::from_chars(health_port_str.data(), health_port_str.data() + health_port_str.size(), configured_port);
        if (ec == std::errc{} && ptr == health_port_str.data() + health_port_str.size() && configured_port > 0 &&
            configured_port <= 65535)
        {
            health_port = configured_port;
        }
    }

    std::string server_address = host + ":" + port;

    auto& redis = varifyservice::VarifyRedisMgr::Instance();
    if (!redis.Ready())
    {
        memolog::LogError("service.dependency_unavailable",
                          "VarifyServer Redis initialization failed",
                          {{"error", redis.StartupError()}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }

    auto& email_task_bus = varifyservice::EmailTaskBus::Instance();
    if (!email_task_bus.StartWorker(nullptr))
    {
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }

    memochat::runtime::ExplicitThread health_thread;
    std::string thread_error;
    if (!health_thread.Start(
            [health_port]()
            {
                RunHealthServer(health_port);
            },
            &thread_error))
    {
        memolog::LogError("service.health_thread_start_failed",
                          "failed to start health endpoint thread",
                          {{"error", thread_error}});
        email_task_bus.StopWorker();
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
    if (!health_thread.Detach(&thread_error))
    {
        memolog::LogError("service.health_thread_detach_failed",
                          "failed to detach health endpoint thread",
                          {{"error", thread_error}});
        std::abort();
    }

    const bool grpc_ok = RunGrpcServer(server_address);

    email_task_bus.StopWorker();
    varifyservice::VarifyRedisMgr::Instance().Close();
    memolog::Telemetry::Shutdown();
    memolog::Logger::Shutdown();
    return grpc_ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

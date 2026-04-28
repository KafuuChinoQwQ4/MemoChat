#include "VarifyServer.h"

#include "VarifyServiceImpl.h"
#include "VarifyRedisMgr.h"
#include "EmailTaskBus.h"
#include "ConfigMgr.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"

#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <grpcpp/grpcpp.h>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;

using tcp = net::ip::tcp;

namespace {

constexpr int DEFAULT_HEALTH_PORT = 8081;

void RunHealthServer(int port) {
    try {
        net::io_context ioc{1};

        tcp::acceptor acceptor(ioc, tcp::endpoint(tcp::v4(), static_cast<unsigned short>(port)));
        memolog::LogInfo("service.health.start", "VarifyServer health endpoint started",
                        {{"address", std::string("0.0.0.0:") + std::to_string(port)}});

        for (;;) {
            tcp::socket socket(ioc);
            acceptor.accept(socket);

            beast::flat_buffer buffer;
            http::request<http::string_body> req;

            try {
                http::read(socket, buffer, req);
            } catch (const std::exception&) {
                beast::error_code ec;
                socket.close(ec);
                continue;
            }

            std::string target = std::string(req.target());
            bool is_health = (target == "/healthz");
            bool is_ready = (target == "/readyz");

            if (!is_health && !is_ready) {
                http::response<http::string_body> res{boost::beast::http::status::not_found, req.version()};
                res.set(http::field::content_type, "text/plain");
                res.body() = "not found";
                res.prepare_payload();
                beast::error_code ec2;
                http::write(socket, res, ec2);
                beast::error_code ec3;
                socket.close(ec3);
                continue;
            }

            bool redis_ok = varifyservice::VarifyRedisMgr::Instance().Ping();

            http::response<http::string_body> res{boost::beast::http::status::ok, req.version()};
            res.set(http::field::content_type, "application/json");

            if (is_ready && !redis_ok) {
                res.result(http::status::service_unavailable);
                res.body() = R"({"status":"unhealthy","reason":"redis_down","service":"VarifyServer"})";
            } else if (is_ready) {
                res.body() = R"({"status":"ready","service":"VarifyServer"})";
            } else {
                res.body() = R"({"status":"ok","service":"VarifyServer"})";
            }

            res.prepare_payload();
            beast::error_code ec2;
            http::write(socket, res, ec2);
            beast::error_code ec3;
            socket.close(ec3);
        }
    } catch (const std::exception& e) {
        memolog::LogError("service.health.error", "health server error",
                         {{"error", e.what()}});
    }
}

void RunGrpcServer(const std::string& address) {
    varifyservice::VarifyServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    memolog::LogInfo("service.start", "VarifyServer listening",
                    {{"address", address}, {"module", "grpc"}});

    net::io_context io_context;
    net::signal_set signals(io_context, SIGINT, SIGTERM);

    signals.async_wait([&server, &io_context](const boost::system::error_code& error, int signal_number) {
        if (!error) {
            memolog::LogInfo("service.stop", "VarifyServer shutting down");
            server->Shutdown();
            io_context.stop();
        }
    });

    std::thread([&io_context]() { io_context.run(); }).detach();

    server->Wait();
}

} // anonymous namespace

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

        memolog::Logger::Init("VarifyServer", log_cfg);
        memolog::Telemetry::Init("VarifyServer", telemetry_cfg);

        std::string host = cfg["VarifyServer"]["Host"];
        std::string port = cfg["VarifyServer"]["Port"];
        std::string health_port_str = cfg["VarifyServer"]["HealthPort"];

        int health_port = DEFAULT_HEALTH_PORT;
        if (!health_port_str.empty()) {
            try { health_port = std::stoi(health_port_str); } catch (...) {}
        }

        std::string server_address = host + ":" + port;

        std::thread health_thread([health_port]() {
            RunHealthServer(health_port);
        });
        health_thread.detach();

        varifyservice::EmailTaskBus::Instance().StartWorker(nullptr);

        RunGrpcServer(server_address);

        varifyservice::EmailTaskBus::Instance().StopWorker();
        varifyservice::VarifyRedisMgr::Instance().Close();
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
    } catch (std::exception const& e) {
        memolog::LogError("service.fatal", "VarifyServer crashed",
                         {{"error", e.what()}});
        varifyservice::EmailTaskBus::Instance().StopWorker();
        varifyservice::VarifyRedisMgr::Instance().Close();
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }

    return 0;
}

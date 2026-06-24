#include "ChatMessageInternalGrpcService.h"
#include "ChatMessageRepository.h"
#include "ChatRelationRepository.h"
#include "ConfigMgr.h"
#include "GroupMessageService.h"
#include "MessageDeliveryService.h"
#include "MongoMgr.h"
#include "PostgresMgr.h"
#include "PrivateMessageService.h"
#include "RedisMgr.h"
#include "RedisOnlineRouteStore.h"
#include "SnowflakeUtil.h"
#include "UserMgr.h"
#include "logging/LogConfig.h"
#include "logging/Logger.h"
#include "logging/Telemetry.h"
#include "logging/TelemetryConfig.h"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <grpcpp/grpcpp.h>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
class ScopeExit
{
public:
    explicit ScopeExit(std::function<void()> fn)
        : _fn(std::move(fn))
    {
    }

    ~ScopeExit()
    {
        if (_fn)
        {
            _fn();
        }
    }

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit& operator=(const ScopeExit&) = delete;

private:
    std::function<void()> _fn;
};

class DisabledMessageEventPublisher final : public IEventPublisher
{
public:
    bool PublishEvent(const std::string& topic, const memochat::json::JsonValue&, std::string* error = nullptr) override
    {
        if (error)
        {
            *error = "ChatMessageService event publishing is disabled in this scaffold";
        }
        memolog::LogWarn("message_service.event_publish_disabled",
                         "ChatMessageService rejected message event publish",
                         {{"topic", topic}});
        return false;
    }
};

std::string ParseConfigPath(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--config")
        {
            if (i + 1 >= argc)
            {
                throw std::runtime_error("missing value for --config");
            }
            return argv[++i];
        }
        throw std::runtime_error("unknown argument: " + arg);
    }
    return "";
}

std::string ConfigValueOrDefault(ConfigMgr& cfg,
                                 const std::string& section,
                                 const std::string& key,
                                 const std::string& default_value)
{
    const auto value = cfg.GetValue(section, key);
    return value.empty() ? default_value : value;
}

void SetInstanceNameEnv(const std::string& instance_name)
{
    if (instance_name.empty())
    {
        return;
    }
#ifdef _WIN32
    _putenv_s("MEMOCHAT_INSTANCE_NAME", instance_name.c_str());
#else
    setenv("MEMOCHAT_INSTANCE_NAME", instance_name.c_str(), 1);
#endif
}

void InitSnowflake(ConfigMgr& cfg)
{
    const auto datacenter_id_str = cfg.GetValue("Snowflake", "DatacenterId");
    const auto worker_id_str = cfg.GetValue("Snowflake", "WorkerId");
    const int64_t datacenter_id = datacenter_id_str.empty() ? 1 : std::stoll(datacenter_id_str);
    const int64_t worker_id = worker_id_str.empty() ? 10 : std::stoll(worker_id_str);
    SnowflakeUtil::getInstance().init(worker_id, datacenter_id);
}

std::string MessageServiceRpcAddress(ConfigMgr& cfg)
{
    const auto host = ConfigValueOrDefault(cfg, "MessageServiceRpc", "Host", "127.0.0.1");
    const auto port = ConfigValueOrDefault(cfg, "MessageServiceRpc", "Port", "50092");
    return host + ":" + port;
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        ConfigMgr::InitConfigPath(ParseConfigPath(argc, argv));
        auto& cfg = ConfigMgr::Inst();

        const auto service_name = ConfigValueOrDefault(cfg, "SelfServer", "Name", "chatmessageservice1");
        SetInstanceNameEnv(service_name);
        InitSnowflake(cfg);

        const auto log_cfg = memolog::LogConfig::FromGetter(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            });
        const auto telemetry_cfg = memolog::TelemetryConfig::FromGetter(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            });
        memolog::Logger::Init("ChatMessageService", log_cfg);
        memolog::Telemetry::Init("ChatMessageService", telemetry_cfg);

        ScopeExit cleanup(
            [service_name]()
            {
                RedisMgr::GetInstance()->DelCount(service_name);
                RedisMgr::GetInstance()->Close();
                memolog::Telemetry::Shutdown();
                memolog::Logger::Shutdown();
            });

        const auto storage_init_start = std::chrono::steady_clock::now();
        PostgresMgr::GetInstance();
        MongoMgr::GetInstance();
        memolog::LogInfo("message_service.storage_ready",
                         "ChatMessageService storage ready",
                         {{"name", service_name},
                          {"storage_init_ms",
                           std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::steady_clock::now() - storage_init_start)
                                              .count())}});

        ChatMessageRepository message_repository;
        ChatRelationRepository relation_repository;
        RedisOnlineRouteStore online_route_store;
        DisabledMessageEventPublisher event_publisher;
        MessageDeliveryService delivery_gateway(nullptr, UserMgr::GetInstance().get(), &online_route_store);
        PrivateMessageService private_message_service(UserMgr::GetInstance().get(),
                                                      &online_route_store,
                                                      &message_repository,
                                                      &relation_repository,
                                                      &delivery_gateway,
                                                      &event_publisher);
        GroupMessageService group_message_service(&message_repository,
                                                  &relation_repository,
                                                  &delivery_gateway,
                                                  &event_publisher);
        ChatMessageInternalGrpcService message_grpc_service(&private_message_service, &group_message_service);

        const auto server_address = MessageServiceRpcAddress(cfg);
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(
            static_cast<chatinternal::ChatPrivateMessageInternalService::Service*>(&message_grpc_service));
        builder.RegisterService(
            static_cast<chatinternal::ChatGroupMessageInternalService::Service*>(&message_grpc_service));

        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        if (!server)
        {
            throw std::runtime_error("failed to start ChatMessageService on " + server_address);
        }

        memolog::LogInfo("message_service.start",
                         "ChatMessageService listening",
                         {{"name", service_name}, {"rpc_address", server_address}});

        boost::asio::io_context io_context;
        boost::asio::signal_set signals(io_context
#ifdef _WIN32
                                        ,
                                        SIGINT,
                                        SIGBREAK
#else
                                        ,
                                        SIGINT,
                                        SIGTERM
#endif
        );
        signals.async_wait(
            [&io_context, &server](auto, auto)
            {
                if (server)
                {
                    server->Shutdown();
                }
                io_context.stop();
            });
        std::thread signal_thread(
            [&io_context]()
            {
                io_context.run();
            });

        server->Wait();
        io_context.stop();
        signal_thread.join();

        memolog::LogInfo("message_service.stop", "ChatMessageService stopped", {{"name", service_name}});
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ChatMessageService fatal: " << e.what() << std::endl;
        memolog::LogError("message_service.fatal", "ChatMessageService crashed", {{"error", e.what()}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
}

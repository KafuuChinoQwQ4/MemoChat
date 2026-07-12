#include "ChatMessageInternalGrpcService.hpp"
#include "ChatMessageRepository.hpp"
#include "ChatRelationRepository.hpp"
#include "ConfigMgr.hpp"
#include "GroupMessageService.hpp"
#include "MessageDeliveryService.hpp"
#include "MongoMgr.hpp"
#include "PostgresMgr.hpp"
#include "PrivateMessageService.hpp"
#include "RedisMgr.hpp"
#include "RedisOnlineRouteStore.hpp"
#include "SnowflakeUtil.hpp"
#include "UserMgr.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"
#include "runtime/ExplicitThread.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <grpcpp/grpcpp.h>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>

import memochat.chat.message_service_runtime_algorithms;

namespace message_service_modules = memochat::chat::message_service::modules;

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
            *error = message_service_modules::DisabledEventPublishError();
        }
        memolog::LogWarn(message_service_modules::DisabledEventPublishLogEvent(),
                         message_service_modules::DisabledEventPublishLogMessage(),
                         {{"topic", topic}});
        return false;
    }
};

bool ParseConfigPath(int argc, char** argv, std::string& path, std::string& error)
{
    path.clear();
    error.clear();
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (message_service_modules::IsConfigFlag(arg.data(), arg.size()))
        {
            if (message_service_modules::ShouldRejectMissingConfigValue(i + 1 < argc))
            {
                error = message_service_modules::MissingConfigValueMessage();
                return false;
            }
            path = argv[++i];
            continue;
        }
        error = std::string(message_service_modules::UnknownArgumentPrefix()) + arg;
        return false;
    }
    return true;
}

std::string ConfigValueOrDefault(ConfigMgr& cfg,
                                 const std::string& section,
                                 const std::string& key,
                                 const std::string& default_value)
{
    const auto value = cfg.GetValue(section, key);
    return message_service_modules::ShouldUseDefaultConfigValue(value.empty()) ? default_value : value;
}

void SetInstanceNameEnv(const std::string& instance_name)
{
    if (!message_service_modules::ShouldSetInstanceName(instance_name.empty()))
    {
        return;
    }
#ifdef _WIN32
    _putenv_s("MEMOCHAT_INSTANCE_NAME", instance_name.c_str());
#else
    setenv("MEMOCHAT_INSTANCE_NAME", instance_name.c_str(), 1);
#endif
}

bool ParseInt64Or(const std::string& raw, int64_t fallback, int64_t& value)
{
    if (raw.empty())
    {
        value = fallback;
        return true;
    }
    const auto parsed = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    return parsed.ec == std::errc{} && parsed.ptr == raw.data() + raw.size();
}

bool InitSnowflake(ConfigMgr& cfg, std::string& error)
{
    const auto datacenter_id_str = cfg.GetValue("Snowflake", "DatacenterId");
    const auto worker_id_str = cfg.GetValue("Snowflake", "WorkerId");
    int64_t datacenter_id = 0;
    int64_t worker_id = 0;
    if (!ParseInt64Or(datacenter_id_str, message_service_modules::DefaultSnowflakeDatacenterId(), datacenter_id) ||
        !ParseInt64Or(worker_id_str, message_service_modules::DefaultSnowflakeWorkerId(), worker_id))
    {
        error = "invalid Snowflake numeric configuration";
        return false;
    }
    if (!SnowflakeUtil::getInstance().init(worker_id, datacenter_id))
    {
        error = "Snowflake WorkerId and DatacenterId must be in range 0..31";
        return false;
    }
    return true;
}

std::string MessageServiceRpcAddress(ConfigMgr& cfg)
{
    const auto host = ConfigValueOrDefault(cfg, "MessageServiceRpc", "Host", message_service_modules::DefaultRpcHost());
    const auto port = ConfigValueOrDefault(cfg, "MessageServiceRpc", "Port", message_service_modules::DefaultRpcPort());
    return host + ":" + port;
}
} // namespace

int main(int argc, char** argv)
{
    std::string config_path;
    std::string startup_error;
    if (!ParseConfigPath(argc, argv, config_path, startup_error))
    {
        std::cerr << "ChatMessageService fatal: " << startup_error << std::endl;
        return EXIT_FAILURE;
    }
    ConfigMgr::InitConfigPath(config_path);
    auto& cfg = ConfigMgr::Inst();

    const auto service_name =
        ConfigValueOrDefault(cfg, "SelfServer", "Name", message_service_modules::DefaultServiceName());
    SetInstanceNameEnv(service_name);
    if (!InitSnowflake(cfg, startup_error))
    {
        std::cerr << "ChatMessageService fatal: " << startup_error << std::endl;
        return EXIT_FAILURE;
    }

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
    if (!memolog::Logger::Init(message_service_modules::LoggerName(), log_cfg, &startup_error))
    {
        std::cerr << "ChatMessageService fatal: " << startup_error << std::endl;
        return EXIT_FAILURE;
    }
    memolog::Telemetry::Init(message_service_modules::LoggerName(), telemetry_cfg);

    ScopeExit cleanup(
        [service_name]()
        {
            RedisMgr::GetInstance()->DelCount(service_name);
            RedisMgr::GetInstance()->Close();
            memolog::Telemetry::Shutdown();
            memolog::Logger::Shutdown();
        });

    const auto storage_init_start = std::chrono::steady_clock::now();
    auto postgres_mgr = PostgresMgr::GetInstance();
    if (!postgres_mgr->Ready())
    {
        memolog::LogError("message_service.postgresql_init_failed",
                          "ChatMessageService PostgreSQL initialization failed",
                          {{"error", postgres_mgr->startupError()}});
        return EXIT_FAILURE;
    }
    MongoMgr::GetInstance();
    memolog::LogInfo("message_service.storage_ready",
                     "ChatMessageService storage ready",
                     {{"name", service_name},
                      {"storage_init_ms",
                       std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::steady_clock::now() - storage_init_start)
                                          .count())}});

    ChatMessageRepository message_repository(*postgres_mgr, *MongoMgr::GetInstance());
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
        memolog::LogError("message_service.start_failed",
                          "failed to start ChatMessageService",
                          {{"rpc_address", server_address}});
        return EXIT_FAILURE;
    }

    memolog::LogInfo("message_service.start",
                     "ChatMessageService listening",
                     {{"name", service_name}, {"rpc_address", server_address}});

    boost::asio::io_context io_context;
    boost::asio::signal_set signals(io_context);
    boost::system::error_code signal_error;
#ifdef _WIN32
    signals.add(SIGINT, signal_error);
    if (!signal_error)
    {
        signals.add(SIGBREAK, signal_error);
    }
#else
    signals.add(SIGINT, signal_error);
    if (!signal_error)
    {
        signals.add(SIGTERM, signal_error);
    }
#endif
    if (signal_error)
    {
        memolog::LogError("message_service.signal_setup_failed",
                          "ChatMessageService failed to register shutdown signals",
                          {{"error", signal_error.message()}});
        server->Shutdown();
        return EXIT_FAILURE;
    }
    signals.async_wait(
        [&io_context, &server](auto, auto)
        {
            if (server)
            {
                server->Shutdown();
            }
            io_context.stop();
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
        memolog::LogError("message_service.thread_start_failed",
                          "ChatMessageService failed to start signal thread",
                          {{"error", thread_error}});
        server->Shutdown();
        return EXIT_FAILURE;
    }

    server->Wait();
    io_context.stop();
    if (!signal_thread.Join(&thread_error))
    {
        memolog::LogError("message_service.thread_join_failed",
                          "ChatMessageService failed to join signal thread",
                          {{"error", thread_error}});
        return EXIT_FAILURE;
    }

    memolog::LogInfo("message_service.stop", "ChatMessageService stopped", {{"name", service_name}});
    return EXIT_SUCCESS;
}

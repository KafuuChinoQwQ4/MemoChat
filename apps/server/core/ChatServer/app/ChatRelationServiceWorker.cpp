#include "ChatRelationInternalGrpcService.hpp"
#include "ChatRelationRepository.hpp"
#include "ChatRelationService.hpp"
#include "ChatRuntime.hpp"
#include "ConfigMgr.hpp"
#include "InlineTaskBus.hpp"
#include "KafkaAsyncEventBus.hpp"
#include "PostgresMgr.hpp"
#include "RabbitMqTaskBus.hpp"
#include "RedisAsyncEventBus.hpp"
#include "RedisMgr.hpp"
#include "RedisRelationBootstrapCache.hpp"
#include "SnowflakeUtil.hpp"
#include "TaskDispatcher.hpp"
#include "ports/IEventPublisher.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"

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

import memochat.chat.relation_service_worker_runtime_algorithms;

namespace relation_service_worker_modules = memochat::chat::relation_service_worker::modules;

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

std::string ParseConfigPath(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (relation_service_worker_modules::IsConfigFlag(arg.data(), arg.size()))
        {
            if (relation_service_worker_modules::ShouldRejectMissingConfigValue(i + 1 < argc))
            {
                throw std::runtime_error(relation_service_worker_modules::MissingConfigValueMessage());
            }
            return argv[++i];
        }
        throw std::runtime_error(std::string(relation_service_worker_modules::UnknownArgumentPrefix()) + arg);
    }
    return "";
}

std::string ConfigValueOrDefault(ConfigMgr& cfg,
                                 const std::string& section,
                                 const std::string& key,
                                 const std::string& default_value)
{
    const auto value = cfg.GetValue(section, key);
    return relation_service_worker_modules::ShouldUseDefaultConfigValue(value.empty()) ? default_value : value;
}

void SetInstanceNameEnv(const std::string& instance_name)
{
    if (!relation_service_worker_modules::ShouldSetInstanceName(instance_name.empty()))
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
    const int64_t datacenter_id = datacenter_id_str.empty()
                                      ? relation_service_worker_modules::DefaultSnowflakeDatacenterId()
                                      : std::stoll(datacenter_id_str);
    const int64_t worker_id =
        worker_id_str.empty() ? relation_service_worker_modules::DefaultSnowflakeWorkerId() : std::stoll(worker_id_str);
    SnowflakeUtil::getInstance().init(worker_id, datacenter_id);
}

std::string RelationServiceRpcAddress(ConfigMgr& cfg)
{
    const auto host =
        ConfigValueOrDefault(cfg, "RelationServiceRpc", "Host", relation_service_worker_modules::DefaultRpcHost());
    const auto port =
        ConfigValueOrDefault(cfg, "RelationServiceRpc", "Port", relation_service_worker_modules::DefaultRpcPort());
    return host + ":" + port;
}

// Thin IEventPublisher that forwards relation-state events straight to the async
// event bus. The full AsyncEventDispatcher also CONSUMES events; this worker is a
// pure producer (the main ChatServer runs the consumers), so it only needs the
// publish side.
class EventBusPublisher : public IEventPublisher
{
public:
    explicit EventBusPublisher(std::shared_ptr<IAsyncEventBus> event_bus)
        : _event_bus(std::move(event_bus))
    {
    }

    bool PublishEvent(const std::string& topic,
                      const memochat::json::JsonValue& payload,
                      std::string* error = nullptr) override
    {
        if (!_event_bus)
        {
            if (error)
            {
                *error = relation_service_worker_modules::EventBusUnavailableError();
            }
            return false;
        }
        return _event_bus->Publish(topic, payload, error);
    }

private:
    std::shared_ptr<IAsyncEventBus> _event_bus;
};

std::shared_ptr<IAsyncTaskBus> BuildTaskBus()
{
    const auto backend = memochat::chatruntime::TaskBusBackend();
    const bool use_rabbitmq = relation_service_worker_modules::IsRabbitMqBackend(backend.data(), backend.size());
    if (use_rabbitmq && RabbitMqTaskBus::BuildAvailable())
    {
        return std::make_shared<RabbitMqTaskBus>();
    }
    if (use_rabbitmq)
    {
        memolog::LogWarn(relation_service_worker_modules::RabbitMqUnavailableLogEvent(),
                         relation_service_worker_modules::RabbitMqUnavailableLogMessage(),
                         {{"configured_backend", backend}});
    }
    return std::make_shared<InlineTaskBus>();
}

std::shared_ptr<IAsyncEventBus> BuildAsyncEventBus()
{
    const auto backend = memochat::chatruntime::AsyncEventBusBackend();
    const bool use_kafka = relation_service_worker_modules::IsKafkaBackend(backend.data(), backend.size());
    if (use_kafka && KafkaAsyncEventBus::BuildAvailable())
    {
        return std::make_shared<KafkaAsyncEventBus>();
    }
    if (use_kafka)
    {
        memolog::LogWarn(relation_service_worker_modules::KafkaUnavailableLogEvent(),
                         relation_service_worker_modules::KafkaUnavailableLogMessage(),
                         {{"configured_backend", backend}});
    }
    return std::make_shared<RedisAsyncEventBus>();
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        ConfigMgr::InitConfigPath(ParseConfigPath(argc, argv));
        auto& cfg = ConfigMgr::Inst();

        const auto service_name =
            ConfigValueOrDefault(cfg, "SelfServer", "Name", relation_service_worker_modules::DefaultServiceName());
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
        memolog::Logger::Init(relation_service_worker_modules::LoggerName(), log_cfg);
        memolog::Telemetry::Init(relation_service_worker_modules::LoggerName(), telemetry_cfg);

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
        memolog::LogInfo("relation_service.postgresql_ready",
                         "ChatRelationServiceWorker postgresql ready",
                         {{"name", service_name},
                          {"storage_init_ms",
                           std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::steady_clock::now() - storage_init_start)
                                              .count())}});

        ChatRelationRepository relation_repository;
        RedisRelationBootstrapCache relation_bootstrap_cache;

        // The accept/add-friend handlers must push a live notification to the
        // online peer (clears the requester's "认证中" without a relogin) and
        // publish a relation-state event for cross-node cache refresh. Before
        // this, the worker passed nullptr publishers, so after the relation
        // command split the peer push was silently dropped. Wire producer-only
        // task + event publishers onto the same buses the main ChatServer
        // consumes (TaskDispatcher handles "relation_notify",
        // AsyncEventDispatcher handles TopicRelationState).
        auto task_bus = BuildTaskBus();
        auto async_event_bus = BuildAsyncEventBus();
        TaskDispatcher task_publisher(
            task_bus,
            []()
            {
                return false;
            },
            nullptr,
            nullptr);
        EventBusPublisher event_publisher(async_event_bus);

        ChatRelationService relation_service(&relation_repository,
                                             &relation_bootstrap_cache,
                                             nullptr,
                                             &task_publisher,
                                             &event_publisher);
        ChatRelationInternalGrpcService relation_grpc_service(&relation_service);

        const auto server_address = RelationServiceRpcAddress(cfg);
        grpc::ServerBuilder builder;
        builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
        builder.RegisterService(&relation_grpc_service);

        std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
        if (!server)
        {
            throw std::runtime_error("failed to start ChatRelationServiceWorker on " + server_address);
        }

        memolog::LogInfo("relation_service.start",
                         "ChatRelationServiceWorker listening",
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

        memolog::LogInfo("relation_service.stop", "ChatRelationServiceWorker stopped", {{"name", service_name}});
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ChatRelationServiceWorker fatal: " << e.what() << std::endl;
        memolog::LogError("relation_service.fatal", "ChatRelationServiceWorker crashed", {{"error", e.what()}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
}

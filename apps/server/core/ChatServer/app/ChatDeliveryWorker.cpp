#include "ChatRuntime.hpp"
#include "ConfigMgr.hpp"
#include "LogicSystem.hpp"
#include "PostgresMgr.hpp"
#include "RedisMgr.hpp"
#include "SnowflakeUtil.hpp"
#include "cluster/ChatClusterDiscovery.hpp"
#include "const.hpp"
#include "logging/LogConfig.hpp"
#include "logging/Logger.hpp"
#include "logging/Telemetry.hpp"
#include "logging/TelemetryConfig.hpp"

#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

import memochat.chat.delivery_worker_runtime_algorithms;

namespace delivery_worker_modules = memochat::chat::delivery_worker::modules;

namespace
{
std::string ParseConfigPath(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (delivery_worker_modules::IsConfigFlag(arg.data(), arg.size()))
        {
            if (delivery_worker_modules::ShouldRejectMissingConfigValue(i + 1 < argc))
            {
                throw std::runtime_error(delivery_worker_modules::MissingConfigValueMessage());
            }
            return argv[++i];
        }
        throw std::runtime_error(std::string(delivery_worker_modules::UnknownArgumentPrefix()) + arg);
    }
    return "";
}

void SetInstanceNameEnv(const std::string& instance_name)
{
    if (!delivery_worker_modules::ShouldSetInstanceName(instance_name.empty()))
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
    const int64_t datacenter_id = datacenter_id_str.empty() ? delivery_worker_modules::DefaultSnowflakeDatacenterId()
                                                            : std::stoll(datacenter_id_str);
    const int64_t worker_id =
        worker_id_str.empty() ? delivery_worker_modules::DefaultSnowflakeWorkerId() : std::stoll(worker_id_str);
    SnowflakeUtil::getInstance().init(worker_id, datacenter_id);
}
} // namespace

int main(int argc, char** argv)
{
    try
    {
        ConfigMgr::InitConfigPath(ParseConfigPath(argc, argv));
        auto& cfg = ConfigMgr::Inst();

        const std::string worker_name = memochat::cluster::ResolveSelfNodeName(
            [&cfg](const std::string& section, const std::string& key)
            {
                return cfg.GetValue(section, key);
            });
        if (delivery_worker_modules::ShouldRejectEmptyWorkerName(worker_name.empty()))
        {
            throw std::runtime_error(delivery_worker_modules::EmptyWorkerNameMessage());
        }
        SetInstanceNameEnv(worker_name);

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
        memolog::Logger::Init(delivery_worker_modules::LoggerName(), log_cfg);
        memolog::Telemetry::Init(delivery_worker_modules::LoggerName(), telemetry_cfg);

        const auto storage_init_start = std::chrono::steady_clock::now();
        PostgresMgr::GetInstance();
        memolog::LogInfo("delivery_worker.postgresql_ready",
                         "ChatDeliveryWorker postgresql ready",
                         {{"name", worker_name},
                          {"storage_init_ms",
                           std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                                              std::chrono::steady_clock::now() - storage_init_start)
                                              .count())}});

        Defer cleanup(
            [worker_name]()
            {
                RedisMgr::GetInstance()->DelCount(worker_name);
                RedisMgr::GetInstance()->Close();
                memolog::Telemetry::Shutdown();
                memolog::Logger::Shutdown();
            });

        auto logic = LogicSystem::GetInstance();
        memolog::LogInfo(
            "delivery_worker.start",
            "ChatDeliveryWorker started",
            {{"name", worker_name},
             {"worker_enabled", delivery_worker_modules::WorkerEnabledText(memochat::chatruntime::IsWorkerEnabled())}});

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
            [&io_context, logic](auto, auto)
            {
                io_context.stop();
            });

        io_context.run();
        memolog::LogInfo("delivery_worker.stop", "ChatDeliveryWorker stopped", {{"name", worker_name}});
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "ChatDeliveryWorker fatal: " << e.what() << std::endl;
        memolog::LogError("delivery_worker.fatal", "ChatDeliveryWorker crashed", {{"error", e.what()}});
        memolog::Telemetry::Shutdown();
        memolog::Logger::Shutdown();
        return EXIT_FAILURE;
    }
}
